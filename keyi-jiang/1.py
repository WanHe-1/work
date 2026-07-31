from __future__ import annotations

import argparse
import os
import sys
import threading
import time
from concurrent.futures import Future, ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import cv2
import numpy as np

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    Image = None
    ImageDraw = None
    ImageFont = None


IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tif", ".tiff"}
WINDOW_NAME = "OpenCV Image Matcher"
MIN_GOOD_MATCHES = 10
MIN_INLIERS = 8
MIN_INLIER_RATIO = 0.40
SERIAL_TRIGGER_DEFAULT = 65.0
SERIAL_REARM_MARGIN = 5.0
DEFAULT_SCAN_INTERVAL = 3


@dataclass
class ReferenceImage:
    path: Path
    keypoints: list[Any]
    descriptors: np.ndarray
    width: int
    height: int


@dataclass
class MatchResult:
    reference: ReferenceImage
    similarity: float
    good_matches: int
    inliers: int
    corners: np.ndarray


def parse_args() -> argparse.Namespace:
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="使用摄像头匹配本地参考图片")
    parser.add_argument(
        "--folder",
        type=Path,
        default=script_dir / "fuzhou",
        help="参考图片文件夹，默认是脚本旁的 fuzhou 文件夹",
    )
    parser.add_argument("--camera", type=int, default=0, help="摄像头编号，默认是 0")
    parser.add_argument(
        "--threshold",
        type=float,
        default=40.0,
        help="显示匹配结果的最低相似度，默认是 40",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=960,
        help="识别画面宽度，默认是 960；降低可提升速度",
    )
    parser.add_argument(
        "--scan-interval",
        type=int,
        default=DEFAULT_SCAN_INTERVAL,
        help="每隔多少帧识别一次，默认是 3；提高可让预览更流畅",
    )
    parser.add_argument(
        "--fps-hint",
        type=float,
        default=15.0,
        help="摄像头读帧目标 FPS（仅限摄像头线程读取间隔，默认 15）",
    )
    parser.add_argument(
        "--port",
        type=str,
        default="",
        help="Arduino 串口，例如 COM3；留空则自动扫描",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=9600,
        help="串口波特率，默认 9600",
    )
    parser.add_argument(
        "--trigger",
        type=float,
        default=SERIAL_TRIGGER_DEFAULT,
        help="触发 Arduino 的相似度阈值（百分比），默认 65",
    )
    parser.add_argument(
        "--no-serial",
        action="store_true",
        help="禁用串口通知，仅在屏幕显示匹配结果",
    )
    return parser.parse_args()


@dataclass
class SerialNotifier:
    port: str
    baudrate: int
    trigger: float
    enabled: bool

    handle: Any = None
    armed: bool = True

    def open(self) -> bool:
        if not self.enabled:
            print("串口通知已禁用（--no-serial）")
            return False

        if serial is None:
            print("未安装 pyserial，无法发送串口数据。可通过 pip install pyserial 启用。")
            self.enabled = False
            return False

        target_port = self.port or self._guess_port()
        if not target_port:
            print("未找到可用串口，跳过串口通知。")
            self.enabled = False
            return False

        try:
            self.handle = serial.Serial(target_port, self.baudrate, timeout=0.1)
        except (OSError, serial.SerialException) as error:
            print(f"无法打开串口 {target_port}：{error}")
            self.enabled = False
            return False

        time.sleep(0.2)
        try:
            self.handle.reset_input_buffer()
            self.handle.reset_output_buffer()
        except Exception:
            pass
        self.port = target_port
        print(f"串口已打开：{target_port} @ {self.baudrate}")
        return True

    @staticmethod
    def _guess_port() -> str:
        candidates = list_ports.comports() if list_ports else []
        for candidate in candidates:
            descriptor = " ".join(
                [
                    candidate.device or "",
                    candidate.description or "",
                    getattr(candidate, "manufacturer", "") or "",
                ]
            ).lower()
            if any(
                token in descriptor
                for token in ("arduino", "ch340", "cp210", "ftdi", "usb-serial")
            ):
                return candidate.device
        if candidates:
            return candidates[0].device
        return ""

    def notify(self, similarity: float | None) -> bool:
        if not self.enabled or self.handle is None:
            return False

        score = similarity if similarity is not None else 0.0
        if self.armed and score >= self.trigger:
            try:
                self.handle.write(b"1")
                self.armed = False
                return True
            except (OSError, serial.SerialException) as error:
                print(f"串口发送失败：{error}")
                self.enabled = False
                return False

        rearm_threshold = max(0.0, self.trigger - SERIAL_REARM_MARGIN)
        if not self.armed and score < rearm_threshold:
            self.armed = True
        return False

    def close(self) -> None:
        if self.handle is not None:
            try:
                self.handle.close()
            except Exception:
                pass
            self.handle = None
            print("串口已关闭。")


def create_feature_detector() -> tuple[Any, int, float, str]:
    if hasattr(cv2, "SIFT_create"):
        detector = cv2.SIFT_create(nfeatures=2500, contrastThreshold=0.03)
        return detector, cv2.NORM_L2, 0.72, "SIFT"

    detector = cv2.ORB_create(
        nfeatures=2500,
        scaleFactor=1.2,
        nlevels=8,
        edgeThreshold=19,
        fastThreshold=12,
    )
    return detector, cv2.NORM_HAMMING, 0.75, "ORB"


def read_image_unicode(path: Path) -> np.ndarray | None:
    try:
        encoded = np.fromfile(str(path), dtype=np.uint8)
    except OSError:
        return None
    if encoded.size == 0:
        return None

    buffer = cv2.imdecode(encoded, cv2.IMREAD_UNCHANGED)
    if buffer is None:
        return None
    return flatten_alpha_to_white(buffer)


def flatten_alpha_to_white(buffer: np.ndarray) -> np.ndarray | None:
    if buffer.ndim == 2:
        return cv2.cvtColor(buffer, cv2.COLOR_GRAY2BGR)

    if buffer.ndim != 3 or buffer.shape[2] not in (3, 4):
        return None

    bgr = buffer[..., :3]
    if buffer.shape[2] == 3:
        return bgr

    alpha = buffer[..., 3:4].astype(np.float32) / 255.0
    foreground = bgr.astype(np.float32) * alpha
    background = np.full_like(foreground, 255.0)
    merged = foreground + background * (1.0 - alpha)
    return np.clip(merged, 0, 255).astype(np.uint8)


def enhance_for_detection(image: np.ndarray) -> np.ndarray:
    if image.ndim == 3:
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    else:
        gray = image

    if max(gray.shape[:2]) < 1200:
        return cv2.GaussianBlur(gray, (3, 3), 0)
    return cv2.GaussianBlur(gray, (5, 5), 0)


def enhance_reference_for_detection(image: np.ndarray) -> np.ndarray:
    if image.ndim == 3:
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    else:
        gray = image

    _, binary = cv2.threshold(
        gray,
        0,
        255,
        cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU,
    )
    if int(binary.mean()) > 127:
        binary = cv2.bitwise_not(binary)

    has_content = float(binary.mean() / 255.0)
    if has_content < 0.005:
        return gray

    kernel_size = 3 if max(gray.shape[:2]) < 800 else 5
    binary = cv2.morphologyEx(
        binary,
        cv2.MORPH_CLOSE,
        cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kernel_size, kernel_size)),
    )

    white_canvas = np.full_like(binary, 255)
    inverted = cv2.bitwise_not(binary)
    sharpened = cv2.addWeighted(white_canvas, 0.75, inverted, 0.25, 0)

    if max(gray.shape[:2]) < 1200:
        return cv2.GaussianBlur(sharpened, (3, 3), 0)
    return cv2.GaussianBlur(sharpened, (5, 5), 0)


def resize_for_features(image: np.ndarray, max_side: int = 1200) -> np.ndarray:
    height, width = image.shape[:2]
    longest_side = max(height, width)
    if longest_side <= max_side:
        return image

    scale = max_side / longest_side
    return cv2.resize(
        image,
        (max(1, int(width * scale)), max(1, int(height * scale))),
        interpolation=cv2.INTER_AREA,
    )


def load_reference_images(folder: Path, detector: Any) -> list[ReferenceImage]:
    if not folder.exists():
        raise FileNotFoundError(f"参考图片文件夹不存在：{folder}")
    if not folder.is_dir():
        raise NotADirectoryError(f"参考图片路径不是文件夹：{folder}")

    image_paths = sorted(
        (
            path
            for path in folder.rglob("*")
            if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS
        ),
        key=lambda path: str(path).lower(),
    )

    references: list[ReferenceImage] = []
    for path in image_paths:
        image = read_image_unicode(path)
        if image is None:
            print(f"跳过无法读取的图片：{path}")
            continue

        image = resize_for_features(image)
        detection_input = enhance_reference_for_detection(image)
        keypoints, descriptors = detector.detectAndCompute(detection_input, None)
        if descriptors is None or len(keypoints) < MIN_GOOD_MATCHES:
            print(
                f"跳过特征点过少的图片：{path.name} "
                f"(检测到 {0 if descriptors is None else len(keypoints)} 个)"
            )
            continue

        height, width = detection_input.shape[:2]
        references.append(
            ReferenceImage(
                path=path,
                keypoints=keypoints,
                descriptors=descriptors,
                width=width,
                height=height,
            )
        )

    return references


def resize_camera_frame(frame: np.ndarray, target_width: int) -> np.ndarray:
    if target_width <= 0 or frame.shape[1] <= target_width:
        return frame
    scale = target_width / frame.shape[1]
    target_height = max(1, int(frame.shape[0] * scale))
    return cv2.resize(frame, (target_width, target_height), interpolation=cv2.INTER_AREA)


def calculate_similarity(
    good_match_count: int,
    inlier_count: int,
    descriptor_count: int,
) -> float:
    inlier_ratio = inlier_count / max(good_match_count, 1)
    inlier_strength = min(inlier_count / 40.0, 1.0)
    reference_coverage = good_match_count / max(min(descriptor_count, 400), 1)
    normalized_coverage = min(reference_coverage / 0.12, 1.0)

    score = (
        0.50 * inlier_ratio
        + 0.35 * inlier_strength
        + 0.15 * normalized_coverage
    )
    return float(np.clip(score * 100.0, 0.0, 100.0))


def match_reference(
    reference: ReferenceImage,
    frame_keypoints: list[Any],
    frame_descriptors: np.ndarray,
    matcher: Any,
    ratio_threshold: float,
    frame_shape: tuple[int, ...],
) -> MatchResult | None:
    try:
        pairs = matcher.knnMatch(reference.descriptors, frame_descriptors, k=2)
    except cv2.error:
        return None

    good_matches = [
        first
        for pair in pairs
        if len(pair) == 2
        for first, second in [pair]
        if first.distance < ratio_threshold * second.distance
    ]
    if len(good_matches) < MIN_GOOD_MATCHES:
        return None

    source_points = np.float32(
        [reference.keypoints[match.queryIdx].pt for match in good_matches]
    ).reshape(-1, 1, 2)
    target_points = np.float32(
        [frame_keypoints[match.trainIdx].pt for match in good_matches]
    ).reshape(-1, 1, 2)

    homography, mask = cv2.findHomography(
        source_points,
        target_points,
        cv2.RANSAC,
        5.0,
    )
    if homography is None or mask is None:
        return None

    inlier_count = int(mask.ravel().sum())
    inlier_ratio = inlier_count / len(good_matches)
    if inlier_count < MIN_INLIERS or inlier_ratio < MIN_INLIER_RATIO:
        return None

    source_corners = np.float32(
        [
            [0, 0],
            [reference.width - 1, 0],
            [reference.width - 1, reference.height - 1],
            [0, reference.height - 1],
        ]
    ).reshape(-1, 1, 2)
    corners = cv2.perspectiveTransform(source_corners, homography)
    if not np.isfinite(corners).all():
        return None

    frame_height, frame_width = frame_shape[:2]
    projected_area = abs(cv2.contourArea(corners))
    frame_area = frame_height * frame_width
    if projected_area < frame_area * 0.01 or projected_area > frame_area * 2.0:
        return None

    similarity = calculate_similarity(
        len(good_matches),
        inlier_count,
        len(reference.descriptors),
    )
    return MatchResult(
        reference=reference,
        similarity=similarity,
        good_matches=len(good_matches),
        inliers=inlier_count,
        corners=corners,
    )


def find_best_match(
    frame: np.ndarray,
    references: list[ReferenceImage],
    detector: Any,
    matcher: Any,
    ratio_threshold: float,
) -> MatchResult | None:
    gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    detection_input = enhance_for_detection(gray_frame)
    frame_keypoints, frame_descriptors = detector.detectAndCompute(detection_input, None)
    if frame_descriptors is None or len(frame_keypoints) < MIN_GOOD_MATCHES:
        return None

    best_match: MatchResult | None = None
    for reference in references:
        result = match_reference(
            reference,
            frame_keypoints,
            frame_descriptors,
            matcher,
            ratio_threshold,
            frame.shape,
        )
        if result is not None and (
            best_match is None or result.similarity > best_match.similarity
        ):
            best_match = result

    return best_match


def find_unicode_font(size: int) -> Any | None:
    if ImageFont is None:
        return None

    windows_dir = Path(os.environ.get("WINDIR", r"C:\Windows"))
    candidates = [
        windows_dir / "Fonts" / "msyh.ttc",
        windows_dir / "Fonts" / "simhei.ttf",
        windows_dir / "Fonts" / "simsun.ttc",
        Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"),
        Path("/System/Library/Fonts/PingFang.ttc"),
    ]
    for font_path in candidates:
        if font_path.exists():
            try:
                return ImageFont.truetype(str(font_path), size=size)
            except OSError:
                continue
    return ImageFont.load_default()


def draw_match_panel(frame: np.ndarray, result: MatchResult) -> np.ndarray:
    filename = result.reference.path.name
    line_one = f"匹配文件：{filename}"
    line_two = f"相似度：{result.similarity:.1f}%"

    if Image is None or ImageDraw is None:
        safe_name = filename.encode("ascii", "backslashreplace").decode("ascii")
        cv2.rectangle(frame, (12, 12), (min(frame.shape[1] - 12, 700), 102), (20, 20, 20), -1)
        cv2.putText(
            frame,
            f"File: {safe_name}",
            (24, 48),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (80, 255, 80),
            2,
            cv2.LINE_AA,
        )
        cv2.putText(
            frame,
            f"Similarity: {result.similarity:.1f}%",
            (24, 84),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (80, 255, 80),
            2,
            cv2.LINE_AA,
        )
        return frame

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image = Image.fromarray(rgb_frame)
    draw = ImageDraw.Draw(image, "RGBA")
    font = find_unicode_font(26)
    draw.rounded_rectangle((12, 12, min(frame.shape[1] - 12, 760), 104), radius=10, fill=(15, 15, 15, 210))
    draw.text((25, 23), line_one, font=font, fill=(90, 255, 110, 255))
    draw.text((25, 62), line_two, font=font, fill=(90, 255, 110, 255))
    return cv2.cvtColor(np.asarray(image), cv2.COLOR_RGB2BGR)


def draw_status(
    frame: np.ndarray,
    reference_count: int,
    detector_name: str,
    scan_interval: int = 1,
) -> None:
    status = (
        f"Scanning {reference_count} images | {detector_name} | "
        f"every {max(1, scan_interval)} frame(s) | Q/ESC: quit | R: reload"
    )
    cv2.rectangle(
        frame,
        (0, frame.shape[0] - 34),
        (frame.shape[1], frame.shape[0]),
        (20, 20, 20),
        -1,
    )
    cv2.putText(
        frame,
        status,
        (12, frame.shape[0] - 10),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.55,
        (230, 230, 230),
        1,
        cv2.LINE_AA,
    )


def open_camera(
    camera_index: int,
) -> tuple[cv2.VideoCapture | None, str, np.ndarray | None]:
    if sys.platform == "win32":
        backends = [
            (cv2.CAP_MSMF, "MSMF"),
            (cv2.CAP_DSHOW, "DSHOW"),
            (cv2.CAP_ANY, "AUTO"),
        ]
    else:
        backends = [(cv2.CAP_ANY, "AUTO")]

    for backend, backend_name in backends:
        print(f"正在尝试打开摄像头 {camera_index}（{backend_name}）...")
        capture = cv2.VideoCapture()
        if hasattr(cv2, "CAP_PROP_OPEN_TIMEOUT_MSEC"):
            capture.set(cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 3000)
        if hasattr(cv2, "CAP_PROP_READ_TIMEOUT_MSEC"):
            capture.set(cv2.CAP_PROP_READ_TIMEOUT_MSEC, 3000)

        try:
            opened = capture.open(camera_index, backend)
        except cv2.error:
            opened = False
        if not opened or not capture.isOpened():
            capture.release()
            continue

        capture.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

        first_frame: np.ndarray | None = None
        start_time = time.monotonic()
        while time.monotonic() - start_time < 3.0:
            success, candidate = capture.read()
            if success and candidate is not None:
                first_frame = candidate
                break
            time.sleep(0.05)

        if first_frame is None:
            print(f"{backend_name} 已打开但无法读取画面，尝试其他后端。")
            capture.release()
            continue

        print(f"摄像头已打开：编号 {camera_index}，后端 {backend_name}")
        return capture, backend_name, first_frame

    return None, "", None


class CameraStream:
    """后台线程拉流，主线程只读最新帧，避免 read() 阻塞主循环。"""

    def __init__(self, capture: cv2.VideoCapture, min_interval: float = 0.05) -> None:
        self.capture = capture
        self.min_interval = min_interval
        self.lock = threading.Lock()
        self._frame: np.ndarray | None = None
        self._stop = threading.Event()
        self._ready = threading.Event()
        self._error: Exception | None = None
        self._thread = threading.Thread(
            target=self._reader,
            name="camera-reader",
            daemon=True,
        )

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._thread.is_alive():
            self._thread.join(timeout=1.0)

    def get_latest(self) -> tuple[bool, np.ndarray | None]:
        if self._error is not None:
            return False, None
        with self.lock:
            frame = self._frame
        if frame is None:
            return False, None
        return True, frame.copy()

    def wait_first(self, timeout: float = 3.0) -> tuple[bool, np.ndarray | None]:
        ready = self._ready.wait(timeout=timeout)
        if not ready:
            return False, None
        with self.lock:
            frame = self._frame
        return frame is not None, (None if frame is None else frame.copy())

    def _reader(self) -> None:
        last_read = 0.0
        while not self._stop.is_set():
            now = time.monotonic()
            if now - last_read < self.min_interval:
                time.sleep(0.005)
                continue
            try:
                success, frame = self.capture.read()
            except cv2.error as error:
                self._error = error
                return
            last_read = now
            if not success or frame is None:
                time.sleep(0.02)
                continue
            with self.lock:
                self._frame = frame
            self._ready.set()


def run() -> int:
    args = parse_args()
    folder = args.folder.expanduser().resolve()
    detector, norm_type, ratio_threshold, detector_name = create_feature_detector()

    try:
        references = load_reference_images(folder, detector)
    except (FileNotFoundError, NotADirectoryError) as error:
        print(error)
        print("请创建该文件夹并放入 jpg、png、bmp、webp 或 tif 图片后重新运行。")
        return 1

    if not references:
        print(f"文件夹中没有可用于识别的图片：{folder}")
        print("请放入清晰且包含足够纹理特征的图片后重新运行。")
        return 1

    print(f"已载入 {len(references)} 张参考图片，特征算法：{detector_name}")
    print("按 Q 或 ESC 退出，按 R 重新载入参考图片。")

    capture, camera_backend, first_frame = open_camera(args.camera)
    if capture is None or first_frame is None:
        print(f"无法打开编号为 {args.camera} 的摄像头。")
        print("请关闭正在使用摄像头的软件，或尝试：python 1.py --camera 1")
        return 1

    scan_interval = max(1, args.scan_interval)
    cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
    preview = resize_camera_frame(first_frame, args.width)
    draw_status(preview, len(references), detector_name, scan_interval)
    cv2.imshow(WINDOW_NAME, preview)
    cv2.waitKey(1)
    print(f"摄像头预览已启动（{camera_backend}）。")

    serial_notifier = SerialNotifier(
        port=args.port,
        baudrate=args.baud,
        trigger=args.trigger,
        enabled=not args.no_serial,
    )
    serial_notifier.open()

    matcher = cv2.BFMatcher(norm_type, crossCheck=False)
    executor = ThreadPoolExecutor(max_workers=1, thread_name_prefix="image-matcher")
    recognition_future: Future[MatchResult | None] | None = None
    current_result: MatchResult | None = None
    frame_number = 0
    stream = CameraStream(capture, min_interval=1.0 / max(1.0, args.fps_hint))
    stream.start()

    try:
        while True:
            success, frame = stream.get_latest()
            if not success or frame is None:
                if stream._error is not None:
                    print(f"摄像头拉流失败：{stream._error}")
                    break
                key = cv2.waitKey(1) & 0xFF
                if key in (ord("q"), ord("Q"), 27):
                    break
                continue

            frame = resize_camera_frame(frame, args.width)

            if recognition_future is not None and recognition_future.done():
                try:
                    current_result = recognition_future.result()
                except Exception as error:
                    print(f"图像识别失败：{error}")
                    current_result = None
                recognition_future = None

                similarity = (
                    current_result.similarity if current_result is not None else None
                )
                if serial_notifier.notify(similarity) and current_result is not None:
                    print(
                        f"串口输出 1：{current_result.reference.path.name}，"
                        f"相似度 {current_result.similarity:.1f}%"
                    )

            if recognition_future is None and frame_number % scan_interval == 0:
                recognition_future = executor.submit(
                    find_best_match,
                    frame.copy(),
                    references,
                    detector,
                    matcher,
                    ratio_threshold,
                )

            display_frame = frame.copy()
            if current_result is not None and current_result.similarity >= args.threshold:
                polygon = np.int32(current_result.corners)
                cv2.polylines(
                    display_frame,
                    [polygon],
                    True,
                    (0, 255, 0),
                    3,
                    cv2.LINE_AA,
                )
                display_frame = draw_match_panel(display_frame, current_result)
                try:
                    cv2.setWindowTitle(
                        WINDOW_NAME,
                        f"{WINDOW_NAME} - {current_result.reference.path.name} "
                        f"- {current_result.similarity:.1f}%",
                    )
                except cv2.error:
                    pass
            else:
                try:
                    cv2.setWindowTitle(WINDOW_NAME, WINDOW_NAME)
                except cv2.error:
                    pass

            draw_status(
                display_frame,
                len(references),
                detector_name,
                scan_interval,
            )
            cv2.imshow(WINDOW_NAME, display_frame)
            key = cv2.waitKey(1) & 0xFF

            if key in (ord("q"), ord("Q"), 27):
                break
            if key in (ord("r"), ord("R")):
                if recognition_future is not None:
                    print("识别任务正在运行，请稍后再按 R 重新载入。")
                else:
                    reloaded = load_reference_images(folder, detector)
                    if reloaded:
                        references = reloaded
                        current_result = None
                        print(f"已重新载入 {len(reloaded)} 张参考图片。")
                    else:
                        print("重新载入失败：没有找到可识别的参考图片。")

            frame_number += 1
    except KeyboardInterrupt:
        print("收到退出命令。")
    finally:
        stream.stop()
        capture.release()
        cv2.destroyAllWindows()
        serial_notifier.close()
        executor.shutdown(wait=False, cancel_futures=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(run())
