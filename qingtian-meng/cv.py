import cv2

def main():
    camera_index = 0  
    cap = cv2.VideoCapture(camera_index)

    if not cap.isOpened():
        print(f"err {camera_index}")
        return

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"分辨率: {width}x{height}")

    part_width = width // 3

    regions = [
        {"name": "Left_Part",  "x": 0,          "y": 0, "w": part_width, "h": height},
        {"name": "Center_Part","x": part_width, "y": 0, "w": part_width, "h": height},
        {"name": "Right_Part", "x": part_width*2,"y": 0, "w": part_width, "h": height}
    ]

    for region in regions:
        cv2.namedWindow(region["name"], cv2.WINDOW_NORMAL)
        cv2.resizeWindow(region["name"], region["w"] // 2, region["h"] // 2)

    print(" 'q' ...")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        for region in regions:
            x, y, w, h = region["x"], region["y"], region["w"], region["h"]
            
            if x + w > width:
                w = width - x
            
            roi = frame[y:y+h, x:x+w]
            
            cv2.imshow(region["name"], roi)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()