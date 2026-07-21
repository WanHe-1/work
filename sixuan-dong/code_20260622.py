import requests
import csv
import time

# 请求头（替换成你自己的 cookie）
headers = {
    "user-agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36",
    "x-requested-with": "XMLHttpRequest",
    "accept": "application/json, text/plain, */*",
    "referer": "",
    "cookie": "SCF=As0L7ZutJK95CIMorGBdhwh5xJEjFCg60m4gRzwWjCMJsLyCY8HdROdIkrzp-KR0u7yZ7mcgVnOO_kztDbQiSmc.; UOR=,,cn.bing.com; SINAGLOBAL=5361034252773.303.1776518488622; XSRF-TOKEN=SOF9kivvUJOWTCJ4iZeLkFkG; SUBP=0033WrSXqPxfM725Ws9jqgMF55529P9D9WFYGGpW3bJKIUBRABY6ekCJ5JpX5KzhUgL.Fon0eKB01K54e0M2dJLoIp7LxKML1KBLBKnLxKqL1hnLBoMRe02Xe0.71KeN; PC_TOKEN=2a334d1d3c; _s_tentry=cn.bing.com; Apache=2042127690347.5881.1782203448174; ULV=1782203448176:7:1:1:2042127690347.5881.1782203448174:1776518488657; SUB=_2A25HPjQYDeRhGe5N6lYS-S7FyDuIHXVkMsnQrDV8PUNbmtANLWvDkW9Nd9EgWoTo9VEHxNQxpfmIW5ge2MCoIR70; ALF=02_1784795464; WBPSESS=ipLVx0Hqtp4IwvxqxwLttyTQbmAPEMxZvDY2U0digkgiZJ_rHw3_sHkIcOVUHKRXTlKNXhSBtF29WMOdDGz0Vmi7ev6Xp5pWQuOUQ4rTMIF3LOMQa_ghP0ElOcCptZsHPUTmoVpV8wlE95Kb7FQ1UQ==",

    "x-xsrf-token": "SOF9kivvUJOWTCJ4iZeLkFkG"
}

# 单页评论抓取
def get_one_page(params):
    url = "https://weibo.com/ajax/statuses/buildComments"

    resp = requests.get(url, headers=headers, params=params)

    print("请求地址:", resp.url)
    print("状态码:", resp.status_code)

    if resp.status_code != 200:
        print("返回内容:", resp.text)
        print(f"请求失败，状态码:{resp.status_code}")
        return None, 0

    result = resp.json()
    data_list = result.get("data", [])

    if not data_list:
        print("没有获取到评论")
        return None, 0

    for data in data_list:
        data_dict = {
            "screen_name": data["user"]["screen_name"],
            "profile_image_url": data["user"]["profile_image_url"],
            "location": data["user"]["location"],
            "created_time": data["created_at"].replace(" +0800", ""),
            "text": data["text_raw"]
        }

        print(f"昵称: {data_dict['screen_name']}")
        print(f"头像: {data_dict['profile_image_url']}")
        print(f"地址: {data_dict['location']}")
        print(f"发布时间: {data_dict['created_time']}")
        print(f"评论内容: {data_dict['text']}")
        print("=" * 80)

        saveData(data_dict)

    max_id = result.get("max_id", 0)
    return max_id, len(data_list)


# 保存数据到 csv
def saveData(data_dict):
    writer.writerow(data_dict)


# 循环抓取全部评论（最多 600 条）
def get_all_data(params):
    total_comments = 0

    max_id, current_comments = get_one_page(params)
    total_comments += current_comments

    while max_id and total_comments < 600:
        time.sleep(2)
        params["max_id"] = max_id
        max_id, current_comments = get_one_page(params)
        total_comments += current_comments

    print(f"已抓取 {total_comments} 条评论，数据爬取完毕。")


if __name__ == "__main__":
    # 固定参数（直接写死，不用输入）
    uid = "6865526227"
    id_str = "5308594150442522"
    fileName = "weibo_comments"

    # 设置 referer
    headers["referer"] = f"https://weibo.com/{uid}/{id_str}"

    # 微博真实 ID（直接用）
    real_id = id_str

    # CSV 表头
    header = ["screen_name", "profile_image_url", "location", "created_time", "text"]

    # 保存到当前目录
    f = open(f"{fileName}.csv", "w", encoding="utf-8-sig", newline="")
    writer = csv.DictWriter(f, fieldnames=header)
    writer.writeheader()

    # 请求参数
    params = {
        "flow": 0,
        "is_reload": 1,
        "id": real_id,
        "is_show_bulletin": 2,
        "is_mix": 0,
        "fetch_level": 0,
        "max_id": 0,
        "count": 20,
        "uid": int(uid)
    }

    # 开始爬取
    get_all_data(params)

    f.close()
    print("全部数据保存完成")