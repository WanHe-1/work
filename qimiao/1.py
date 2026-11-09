def Recognize_Dish(self,img):
  #-------------------香蕉检测-----------------#
  banana_num = 0
  hsv_img=cv2.cvtColor(img,cv2.COLOR_BGR2HSV)
  lower_yellow = np.array([15,30,145])#颜色范围低阈值
  upper_yellow = np.array([35,255,255])#颜色范围高阈值
  mask = cv2.inRange(hsv_img,lower_yellow,upper_yellow)#根据颜色范围删选
  mask = cv2.medianBlur(mask, 5)#中值滤波
  #cv2.imshow('mask_banana', mask)
  contours,hierarchy = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt in contours:
    rect = cv2.minAreaRect(cnt)
    box = cv2.boxPoints(rect)
    box = np.int0(box)
    width = max(rect[1][0],rect[1][1])
    height = min(rect[1][0],rect[1][1])
    center = (int(rect[0][0]),int(rect[0][1]))
    if width > 180 and height > 80 and height < 130:
      #print(width,height)
      img = cv2.drawContours(img,[box],0,(0,0,255),2)
      cv2.putText(img,'banana',center,font,1,(255,0,255), 2)
      banana_num += 1
  item_0 = QTableWidgetItem("%d"%banana_num)
  self.tableWidget.setItem(8, 0, item_0)

  #-------------------苹果检测-----------------#
  apple_num = 0
  lower_apple = np.array([0,50,50])#颜色范围低阈值
  upper_apple = np.array([30,255,255])#颜色范围高阈值
  mask_apple = cv2.inRange(hsv_img,lower_apple,upper_apple)#根据颜色范围删选
  mask_apple = cv2.medianBlur(mask_apple, 9)#中值滤波
  #cv2.imshow('mask_apple', mask_apple)
  #cv2.imwrite('mask_apple.jpg', mask_apple)
  contours2,hierarchy2 = cv2.findContours(mask_apple, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt2 in contours2:
    center,radius = cv2.minEnclosingCircle(cnt2)
    area = cv2.contourArea(cnt2)
    #print(radius)  
    rate = area / (math.pi * radius *radius)
    
    if radius > 50 and radius < 75 and rate < 0.91:
      #print(radius)
      cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(0,255,0),2)
      cv2.putText(img,'apple',(int(center[0]),int(center[1])),font,1,(255,0,0), 2)
      apple_num += 1
  item_1 = QTableWidgetItem("%d"%apple_num)
  self.tableWidget.setItem(6, 0, item_1) 

  #-------------------橘子检测-----------------#
  orange_num = 0
  lower_orange = np.array([0,90,60])#颜色范围低阈值
  upper_orange = np.array([60,255,255])#颜色范围高阈值
  mask_orange = cv2.inRange(hsv_img,lower_orange,upper_orange)#根据颜色范围删选
  mask_orange = cv2.medianBlur(mask_orange, 5)#中值滤波
  #cv2.imshow('mask_orange', mask_orange)
  #cv2.imwrite('mask_orange.jpg', mask_orange)
  contours3,hierarchy3 = cv2.findContours(mask_orange, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt3 in contours3:
    center,radius = cv2.minEnclosingCircle(cnt3)
    area = cv2.contourArea(cnt3)
    #print(radius)
    rate = area / (math.pi * radius *radius)
    if radius > 50 and radius < 75 and rate > 0.85:
      #print(radius)
      cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(255,0,255),2)
      cv2.putText(img,'orange',(int(center[0]),int(center[1])),font,1,(255,255,0), 2)
      orange_num += 1
  item_2 = QTableWidgetItem("%d"%orange_num)
  self.tableWidget.setItem(7, 0, item_2)

  #-------------------白色餐盘检测-----------------#
  white_circle_num = 0
  white_rect_num = 0
  lower_white = np.array([0,0,150])#颜色范围低阈值
  upper_white= np.array([100,55,255])#颜色范围高阈值
  mask_white = cv2.inRange(hsv_img,lower_white,upper_white)#根据颜色范围删选
  mask_white = cv2.medianBlur(mask_white, 5)#中值滤波
  #cv2.imshow('mask_white', mask_white)
  #cv2.imwrite('mask_white.jpg', mask_white)
  contours4,hierarchy4 = cv2.findContours(mask_white, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt4 in contours4:
    area = cv2.contourArea(cnt4)
    center,radius = cv2.minEnclosingCircle(cnt4)
    #print(radius)
    rate = area / (math.pi * radius *radius)
    if radius > 100 and radius < 160:
      #print(radius)
      if rate >= 0.9:
        cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(255,255,0),2)
        cv2.putText(img,'white_circle',(int(center[0]),int(center[1])),font,1,(0,255,0), 2)
        white_circle_num += 1
      elif rate >0.6 and rate < 0.9:
        rect = cv2.minAreaRect(cnt4)
        box = cv2.boxPoints(rect)
        box = np.int0(box)
        #cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(255,0,255),5)
        img = cv2.drawContours(img,[box],0,(255,255,0),2)
        cv2.putText(img,'white_rect',(int(center[0]),int(center[1])),font,1,(0,255,0), 2)
        white_rect_num += 1
  item_3 = QTableWidgetItem("%d"%white_circle_num)
  self.tableWidget.setItem(0, 0, item_3)
  item_4 = QTableWidgetItem("%d"%white_rect_num)
  self.tableWidget.setItem(1, 0, item_4)

  #-------------------绿色餐盘检测-----------------#
  green_circle_num = 0
  green_rect_num = 0
  lower_green = np.array([30,65,65])#颜色范围低阈值
  upper_green= np.array([80,255,255])#颜色范围高阈值
  mask_green = cv2.inRange(hsv_img,lower_green,upper_green)#根据颜色范围删选
  mask_green = cv2.medianBlur(mask_green, 5)#中值滤波
  #cv2.imshow('mask_green', mask_green)
  #cv2.imwrite('mask_green.jpg', mask_green)
  contours5,hierarchy5 = cv2.findContours(mask_green, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt5 in contours5:
    area = cv2.contourArea(cnt5)
    center,radius = cv2.minEnclosingCircle(cnt5)
    #print(radius)
    rate = area / (math.pi * radius *radius)
    if radius > 100 and radius < 160:
      #print(radius)
      if rate >= 0.9:
        cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(0,255,0),2)
        cv2.putText(img,'green_circle',(int(center[0]),int(center[1])),font,1,(0,255,255), 2)
        green_circle_num += 1
      elif rate >0.6 and rate < 0.9:
        rect = cv2.minAreaRect(cnt5)
        box = cv2.boxPoints(rect)
        box = np.int0(box)
        #cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(255,0,255),5)
        img = cv2.drawContours(img,[box],0,(0,255,0),2)
        cv2.putText(img,'green_rect',(int(center[0]),int(center[1])),font,1,(0,255,255), 2)
        green_rect_num += 1
  item_5 = QTableWidgetItem("%d"%green_circle_num)
  self.tableWidget.setItem(4, 0, item_5)
  item_6 = QTableWidgetItem("%d"%green_rect_num)
  self.tableWidget.setItem(5, 0, item_6)

  #-------------------橙色餐盘检测-----------------#
  orange_circle_num = 0
  orange_rect_num = 0
  lower_orange_dish = np.array([0,100,100])#颜色范围低阈值
  upper_orange_dish= np.array([15,255,255])#颜色范围高阈值
  mask_orange_dish = cv2.inRange(hsv_img,lower_orange_dish,upper_orange_dish)#根据颜色范围删选
  mask_orange_dish = cv2.medianBlur(mask_orange_dish, 5)#中值滤波
  #cv2.imshow('mask_green', mask_green)
  #cv2.imwrite('mask_orange_dish.jpg', mask_orange_dish)
  contours6,hierarchy6 = cv2.findContours(mask_orange_dish, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
  for cnt6 in contours6:
    area = cv2.contourArea(cnt6)
    center,radius = cv2.minEnclosingCircle(cnt6)
    #print('----------------')
    #print(radius)
    rate = area / (math.pi * radius *radius)
    if radius > 100 and radius < 160:
      #print(rate)
      if rate >= 0.8:
        cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(0,255,0),2)
        cv2.putText(img,'orange_circle',(int(center[0]),int(center[1])),font,1,(255,0,255), 2)
        orange_circle_num += 1
      elif rate >0.3 and rate < 0.8:
        rect = cv2.minAreaRect(cnt6)
        box = cv2.boxPoints(rect)
        box = np.int0(box)
        #cv2.circle(img,(int(center[0]),int(center[1])),int(radius),(255,0,255),5)
        img = cv2.drawContours(img,[box],0,(0,255,0),2)
        cv2.putText(img,'orange_rect',(int(center[0]),int(center[1])),font,1,(255,0,255), 2)
        orange_rect_num += 1
  item_7 = QTableWidgetItem("%d"%orange_circle_num)
  self.tableWidget.setItem(2, 0, item_7)
  item_8 = QTableWidgetItem("%d"%orange_rect_num)
  self.tableWidget.setItem(3, 0, item_8)

  for i in range(0,9):
    self.tableWidget.item(i,0).setTextAlignment(QtCore.Qt.AlignHCenter|QtCore.Qt.AlignVCenter)
    self.tableWidget.item(i,1).setTextAlignment(QtCore.Qt.AlignHCenter|QtCore.Qt.AlignVCenter)
  #----------------计算价格--------------#     
  self.price =  self.price_white_circle * white_circle_num + \
       self.price_white_rect * white_rect_num + \
       self.price_orange_circle * orange_circle_num + \
       self.price_orange_rect * orange_rect_num + \
       self.price_green_circle * green_circle_num + \
       self.price_green_rect * green_rect_num + \
       self.price_apple * apple_num + \
       self.price_orange * orange_num +\
       self.price_banana * banana_num
  print(self.price)
  return img