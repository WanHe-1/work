
from __future__ import annotations

import argparse
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import cv2
import numpy as np

try:
    import serial
except ImportError:
    serial = None


IMAGE_EXTENSIONS = {
    ".jpg", ".jpeg", ".png", ".bmp",
    ".webp", ".tif", ".tiff"
}

WINDOW_NAME = "符文高光纹理实时识别"
MIN_MATCHES = 6


@dataclass
class Reference:
    path: Path
    keypoints: list[Any]
    descriptors: np.ndarray
    width: int
    height: int


@dataclass
class Match:
    reference: Reference
    score: float
    corners: np.ndarray
    good_matches: int
    inliers: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="摄像头实时符文纹理识别，并通过串口发送信号"
    )

    parser.add_argument(
        "--folder",
        type=Path,
        default=Path(__file__).resolve().parent / "fuzhou"
    )
    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--threshold", type=float, default=20.0)
    parser.add_argument("--brightness", type=int, default=145)
    parser.add_argument("--interval", type=float, default=0.25)
    parser.add_argument("--port", default="COM4")
    parser.add_argument("--baud", type=int, default=9600)

    return parser.parse_args()


def read_image(path: Path) -> np.ndarray | None:
    """兼容中文路径和 PNG 图片。"""
    try:
        data = np.fromfile(str(path), dtype=np.uint8)

        if data.size == 0:
            return None

        image = cv2.imdecode(data, cv2.IMREAD_UNCHANGED)

        if image is None:
            return None

        if image.ndim == 2:
            return cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)

        if image.shape[2] == 4:
            bgr = image[:, :, :3].astype(np.float32)
            alpha = image[:, :, 3:4].astype(np.float32) / 255.0

            image = (
                bgr * alpha + 255.0 * (1.0 - alpha)
            ).astype(np.uint8)

        return image[:, :, :3].copy()

    except (OSError, cv2.error, ValueError) as error:
        print(f"[读取失败] {path.name}: {error}")
        return None


def resize_image(
    image: np.ndarray,
    max_side: int = 1000
) -> np.ndarray:
    height, width = image.shape[:2]
    longest = max(height, width)

    if longest <= max_side:
        return image

    scale = max_side / longest

    return cv2.resize(
        image,
        (
            max(1, int(width * scale)),
            max(1, int(height * scale))
        ),
        interpolation=cv2.INTER_AREA
    )


def to_gray(image: np.ndarray) -> np.ndarray:
    if image.ndim == 2:
        return image.copy()

    return cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)


def extract_texture(
    image: np.ndarray,
    brightness: int = 145
) -> np.ndarray:
    """增强符文线条，不通过亮度掩膜删除图像细节。"""
    gray = to_gray(image)

    clahe = cv2.createCLAHE(
        clipLimit=2.0,
        tileGridSize=(8, 8)
    )

    enhanced = clahe.apply(gray)

    blurred = cv2.GaussianBlur(
        enhanced, (0, 0), 1.0
    )

    sharpened = cv2.addWeighted(
        enhanced, 1.5,
        blurred, -0.5,
        0
    )

    return sharpened


def create_detector():
    if not hasattr(cv2, "SIFT_create"):
        raise RuntimeError(
            "当前 OpenCV 不支持 SIFT，请运行："
            "pip install -U opencv-python"
        )

    return cv2.SIFT_create(
        nfeatures=5000,
        contrastThreshold=0.005,
        edgeThreshold=20,
        sigma=1.2
    )


def load_references(
    folder: Path,
    detector: Any,
    brightness: int
) -> list[Reference]:

    if not folder.is_dir():
        raise FileNotFoundError(
            f"参考图片文件夹不存在：{folder}"
        )

    references: list[Reference] = []

    paths = sorted(
        (
            p for p in folder.rglob("*")
            if p.is_file()
            and p.suffix.lower() in IMAGE_EXTENSIONS
        ),
        key=lambda p: str(p).lower()
    )

    print(f"\n参考图片目录：{folder}")
    print(f"发现图片数量：{len(paths)}\n")

    for path in paths:
        image = read_image(path)

        if image is None:
            print(f"[跳过] 无法读取：{path.name}")
            continue

        image = resize_image(image)
        gray = to_gray(image)
        enhanced = extract_texture(image, brightness)

        keypoints, descriptors = detector.detectAndCompute(
            enhanced, None
        )

        if descriptors is None or len(keypoints) < MIN_MATCHES:
            keypoints, descriptors = detector.detectAndCompute(
                gray, None
            )

        if descriptors is None or len(keypoints) < MIN_MATCHES:
            enlarged = cv2.resize(
                gray,
                None,
                fx=1.5,
                fy=1.5,
                interpolation=cv2.INTER_CUBIC
            )

            keypoints, descriptors = detector.detectAndCompute(
                enlarged, None
            )

            if descriptors is not None:
                keypoints = [
                    cv2.KeyPoint(
                        kp.pt[0] / 1.5,
                        kp.pt[1] / 1.5,
                        kp.size / 1.5,
                        kp.angle,
                        kp.response,
                        kp.octave,
                        kp.class_id
                    )
                    for kp in keypoints
                ]

        count = 0 if descriptors is None else len(descriptors)

        print(
            f"[参考图] {path.name} | "
            f"尺寸：{image.shape[1]}x{image.shape[0]} | "
            f"特征点：{count}"
        )

        if descriptors is None or count < 4:
            print(f"[警告] 特征点不足：{path.name}")
            continue

        height, width = gray.shape[:2]

        references.append(
            Reference(
                path=path,
                keypoints=keypoints,
                descriptors=descriptors,
                width=width,
                height=height
            )
        )

    print(f"\n成功加载参考图片：{len(references)} 张\n")

    return references


def match_one(
    reference: Reference,
    keypoints: list[Any],
    descriptors: np.ndarray,
    matcher: Any,
    frame_shape: tuple[int, ...]
) -> Match | None:

    try:
        pairs = matcher.knnMatch(
            reference.descriptors,
            descriptors,
            k=2
        )
    except cv2.error:
        return None

    good = []

    for pair in pairs:
        if len(pair) != 2:
            continue

        first, second = pair

        if first.distance < 0.85 * second.distance:
            good.append(first)

    if len(good) < MIN_MATCHES:
        return None

    source = np.float32([
        reference.keypoints[m.queryIdx].pt
        for m in good
    ]).reshape(-1, 1, 2)

    target = np.float32([
        keypoints[m.trainIdx].pt
        for m in good
    ]).reshape(-1, 1, 2)

    homography, mask = cv2.findHomography(
        source,
        target,
        cv2.RANSAC,
        5.0
    )

    if homography is None or mask is None:
        return None

    inliers = int(mask.ravel().sum())
    inlier_ratio = inliers / len(good)

    if inliers < MIN_MATCHES or inlier_ratio < 0.25:
        return None

    source_corners = np.float32([
        [0, 0],
        [reference.width - 1, 0],
        [reference.width - 1, reference.height - 1],
        [0, reference.height - 1]
    ]).reshape(-1, 1, 2)

    corners = cv2.perspectiveTransform(
        source_corners,
        homography
    )

    if not np.isfinite(corners).all():
        return None

    frame_height, frame_width = frame_shape[:2]

    if (
        np.abs(corners[:, 0, 0]).max() > frame_width * 2
        or np.abs(corners[:, 0, 1]).max() > frame_height * 2
    ):
        return None

    area = abs(cv2.contourArea(corners))

    if area < frame_height * frame_width * 0.003:
        return None

    score = 100.0 * (
        0.65 * inlier_ratio
        + 0.35 * min(inliers / 25.0, 1.0)
    )

    return Match(
        reference=reference,
        score=float(score),
        corners=corners,
        good_matches=len(good),
        inliers=inliers
    )


def find_match(
    frame: np.ndarray,
    references: list[Reference],
    detector: Any,
    matcher: Any,
    brightness: int
) -> Match | None:

    gray = to_gray(frame)
    texture = extract_texture(gray, brightness)

    keypoints, descriptors = detector.detectAndCompute(
        texture, None
    )

    if descriptors is None or len(keypoints) < MIN_MATCHES:
        keypoints, descriptors = detector.detectAndCompute(
            gray, None
        )

    count = 0 if descriptors is None else len(descriptors)

    print(f"[实时检测] 当前画面特征点：{count}")

    if descriptors is None or count < MIN_MATCHES:
        return None

    best: Match | None = None

    for reference in references:
        candidate = match_one(
            reference,
            keypoints,
            descriptors,
            matcher,
            frame.shape
        )

        if candidate is None:
            continue

        print(
            f"[候选] {reference.path.name} | "
            f"匹配点：{candidate.good_matches} | "
            f"内点：{candidate.inliers} | "
            f"评分：{candidate.score:.1f}"
        )

        if best is None or candidate.score > best.score:
            best = candidate

    if best is None:
        print("[结果] 没有通过几何验证的图片")

    return best


class Camera:
    """独立线程持续读取摄像头。"""

    def __init__(self, capture: cv2.VideoCapture):
        self.capture = capture
        self.lock = threading.Lock()
        self.frame: np.ndarray | None = None
        self.stop_event = threading.Event()

        self.thread = threading.Thread(
            target=self._read_loop,
            daemon=True
        )

    def start(self):
        self.thread.start()

    def latest(self):
        with self.lock:
            if self.frame is None:
                return None
            return self.frame.copy()

    def _read_loop(self):
        while not self.stop_event.is_set():
            success, image = self.capture.read()

            if not success or image is None:
                time.sleep(0.01)
                continue

            with self.lock:
                self.frame = image

    def stop(self):
        self.stop_event.set()
        self.thread.join(timeout=1.0)
        self.capture.release()


def open_camera(index: int):
    for backend in (
        cv2.CAP_DSHOW,
        cv2.CAP_MSMF,
        cv2.CAP_ANY
    ):
        capture = cv2.VideoCapture(index, backend)

        if capture.isOpened():
            capture.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            capture.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            return capture

        capture.release()

    return None


def run() -> int:
    cv2.setNumThreads(1)
    args = parse_args()

    args.threshold = max(
        0.0, min(100.0, args.threshold)
    )
    args.interval = max(0.1, args.interval)

    # 打开串口
    ser = None

    if serial is None:
        print(
            "[串口错误] 未安装 pyserial。"
            "请运行：python -m pip install pyserial"
        )
    else:
        try:
            ser = serial.Serial(
                args.port,
                args.baud,
                timeout=0.1,
                write_timeout=1
            )

            # Arduino UNO 等设备打开串口时可能自动复位
            time.sleep(2)
            ser.reset_input_buffer()

            print(
                f"[串口] 已连接 {args.port}，"
                f"波特率 {args.baud}"
            )

        except (serial.SerialException, OSError) as error:
            print(f"[串口错误] 无法打开 {args.port}: {error}")
            ser = None

    try:
        detector = create_detector()
    except RuntimeError as error:
        print(error)
        if ser is not None:
            ser.close()
        return 1

    folder = args.folder.expanduser().resolve()

    try:
        references = load_references(
            folder,
            detector,
            args.brightness
        )
    except FileNotFoundError as error:
        print(error)
        if ser is not None:
            ser.close()
        return 1

    if not references:
        print("没有可用参考图片，请检查 fuzhou 文件夹。")
        if ser is not None:
            ser.close()
        return 1

    capture = open_camera(args.camera)

    if capture is None:
        print(f"无法打开摄像头：{args.camera}")
        if ser is not None:
            ser.close()
        return 1

    camera = Camera(capture)
    camera.start()

    matcher = cv2.BFMatcher(cv2.NORM_L2)
    executor = ThreadPoolExecutor(max_workers=1)

    future = None
    current: Match | None = None
    last_submit = 0.0
    last_result_time = 0.0

    # 防止同一个持续出现的目标重复发送
    signal_sent = False

    cv2.namedWindow(
        WINDOW_NAME,
        cv2.WINDOW_NORMAL
    )

    print("摄像头已启动，按 Q 或 Esc 退出。")
    print("识别成功时将向串口发送字符 1。")

    try:
        while True:
            frame = camera.latest()

            if frame is None:
                cv2.waitKey(1)
                continue

            if args.width > 0 and frame.shape[1] > args.width:
                scale = args.width / frame.shape[1]

                frame = cv2.resize(
                    frame,
                    (
                        args.width,
                        max(1, int(frame.shape[0] * scale))
                    ),
                    interpolation=cv2.INTER_AREA
                )

            now = time.monotonic()

            # 处理完成的识别任务
            if future is not None and future.done():
                try:
                    current = future.result()
                except Exception as error:
                    print(f"[错误] 后台识别异常：{error}")
                    current = None

                last_result_time = now
                future = None

                detected = (
                    current is not None
                    and current.score >= args.threshold
                )

                if detected and not signal_sent:
                    print(
                        f"[识别成功] {current.reference.path.name}，"
                        f"评分：{current.score:.1f}"
                    )

                    if ser is not None and ser.is_open:
                        try:
                            # 发送单个 ASCII 字符 1，不附加换行
                            written = ser.write(b"1")
                            ser.flush()

                            if written == 1:
                                signal_sent = True
                                print(
                                    f"[串口发送成功] 已向 {args.port} "
                                    f"发送字符 1"
                                )
                            else:
                                print("[串口发送失败] 未发送完整数据")

                        except (serial.SerialException, OSError) as error:
                            print(f"[串口发送失败] {error}")
                    else:
                        print(
                            "[串口发送失败] 串口未连接，"
                            "请检查 COM4 是否被其他程序占用"
                        )

                elif not detected:
                    # 识别结果消失，允许下一次触发
                    signal_sent = False

            # 上一个识别任务结束后才提交新任务
            if (
                future is None
                and now - last_submit >= args.interval
            ):
                future = executor.submit(
                    find_match,
                    frame.copy(),
                    references,
                    detector,
                    matcher,
                    args.brightness
                )

                last_submit = now

            # 摄像头画面仅显示图像与识别成功边框
            display = frame.copy()

            if (
                current is not None
                and current.score >= args.threshold
                and now - last_result_time < 1.5
            ):
                polygon = np.int32(current.corners)

                cv2.polylines(
                    display,
                    [polygon],
                    True,
                    (0, 255, 0),
                    3
                )

            cv2.imshow(WINDOW_NAME, display)

            key = cv2.waitKey(1) & 0xFF

            if key in (27, ord("q"), ord("Q")):
                break

    finally:
        camera.stop()

        executor.shutdown(
            wait=False,
            cancel_futures=True
        )

        cv2.destroyAllWindows()

        if ser is not None and ser.is_open:
            ser.close()
            print("[串口] 已关闭")

    return 0


if __name__ == "__main__":
    raise SystemExit(run())
