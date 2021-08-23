#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#
#  12306.py
#
#  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

"""Order train tickets on 12306 semi-automatically. 半自动12306订票。

Also a little experiment of programming with Chinese characters ;-P.
Feel free to ask me for a translated version.
"""

# Choose any browser available, but be aware that Safari may not
# allow you to take over when this program goes off the rail
from selenium.webdriver import Firefox as 浏览器驱动
from selenium.common.exceptions import JavascriptException as 错误
from selenium.webdriver.common.by import By as 依据
from selenium.webdriver.support.ui import WebDriverWait as 浏览器里等
from selenium.webdriver.support import expected_conditions as 等到
from time import sleep as 等

# Readline makes wide char input() more usable
try:
    import readline
except ImportError:
    pass

# Just to confuse people
说 = print
求输入 = input
统统检查 = enumerate
范围 = range
长度 = len
类型 = type
字典 = dict
整数 = int
我爱你 = True

# WebDriverWait timeout
不耐烦限 = 120

# Delay interval to be less likely to be blocked
友好延时 = 0.1

# AIO JavaScript function to play with the train list page
找火车 = """function afterGetPage(train_i_love) {
    // Remove 温馨提示
    try {
        document.getElementById("qd_closeDefaultWarningWindowDialog_id").click();
    } catch(e){ /* skip */ }

    // Query for 火车
    let tickets = document.querySelector("#t-list table tbody").childNodes;
    let train = undefined;
    tickets.forEach(function(a) {
        let pnode = a.firstChild;
        if (!pnode) { return;/* Skip datatrans */ }
        if (pnode.firstChild.firstChild.firstChild.firstChild.innerText == train_i_love) {
            train = pnode.parentElement;
        }
    });

    // Now train is the specified row
    let yuding_button = train.lastChild.firstChild;
    // Try to click 预定 or refresh
    yuding_button.click();
}"""

# AIO JavaScript function to select seat type
选座 = """
/// abbrev is defined in the Python input()
function chooseSeatType(abbrev) {
    document.querySelectorAll("select").forEach(elem => {
        if (elem.id.startsWith("seatType")) {
            for (let cn in elem.children) {
                if (elem.children[cn].text.includes(abbrev)) {
                    elem.value = elem.children[cn].value;
                    break;
                }
            }
        }
    });
}"""

# AIO JavaScript function to select seat/bed no
选座选床 = """
/// row is 0 or 1, id is '[A-DFa-df]'
function checkSeat(row, id) {
    let rowelem = document.querySelectorAll("#id-seat-sel .seat-sel-bd .sel-item[style*='block']")[row];
    let lis = rowelem.querySelectorAll("ul li");
    for (let li in lis) {
        let anchor = lis[li].firstElementChild;
        if (anchor && anchor.innerText == id) {
            anchor.click();
        }
    }
}

/// pos is x, z or s, num is R >= 0
function checkBed(pos, num) {
    var old=parseInt(document.getElementById(pos+"_no").innerText);
    var diff = num - old;
    if (diff > 0) {
        for (let a=0; a<diff; a++) {
            numSet("add", pos + "_no");
        }
    } else if (diff < 0) {
        for (let a=0; a<-diff; a++) {
            numSet("reduce", pos + "_no");
        }
    }
}
"""

# AIO JavaScript function to click "Submit"
提交 = """
(function() {
    let submit = document.getElementById("qr_submit_id");
    // Click until this dialog disappears
    setTimeout(() => { submit.click(); }, 2000);
})();
"""

def 问座位():
    """Ask for a properlly formatted seat sequence.
    
    The type of the return value reflects the type of tickets (i.e. bed or seat).
    """
    座位选择 = 求输入("坐哪里 [ABCDFabcdf | 上n中n下n, 同大/小写为同排]？")
    第一排座 = [字符 for 字符 in 座位选择 if 字符.isupper()]
    第二排座 = [字符.upper() for 字符 in 座位选择 if 字符.islower()]
    if 第一排座 or 第二排座:
        return (第一排座, 第二排座)
    # Assumes the person, who must be me, knows to enter correctly
    铺位 = {'上': 0, '中': 0, '下': 0}
    逐对 = [座位选择[首:首+2] for 首 in 范围(0, 长度(座位选择), 2)]
    for 个 in 逐对:
        铺位[个[0]] = 整数(个[1])
    # Rename to 12306 style
    铺位['s'] = 铺位.pop('上')
    铺位['z'] = 铺位.pop('中')
    铺位['x'] = 铺位.pop('下')
    return 铺位


# Be informed about the license
说("老张订票 版权所有 (C) 2021")
说("本程序从未提供任何担保。这是款自由软件，你可以在满足一定条件后对其再发布。")
说("请移步 https://www.gnu.org/licenses 以查看详情。")
说("请注意不正确的使用可能导致封号。作者对此不承担任何责任和后果。")
说()

with 浏览器驱动() as 浏览器:
    def 等到你出现(选择器):
        return 浏览器里等(浏览器, 不耐烦限).until(等到.visibility_of_element_located((依据.CSS_SELECTOR, 选择器)))
    # Fire up the browser and login
    浏览器.get("https://kyfw.12306.cn/otn/resources/login.html")
    说("请扫描登录（通常只需一次）！")
    浏览器.maximize_window()

    # Ask for basic information about the ticket to order
    # Starting with https://kyfw.12306.cn/otn/leftTicket/init?linktypeid=dc&
    链接 = 求输入("告诉我链接？")
    # e.g. G1372
    火车次 = 求输入("告诉我火车？").upper()
    # e.g. 张三,李四
    乘客s = [人.strip() for 人 in 求输入("谁要坐车 [英文逗号分隔]？").split(',')]
    席别 = 求输入(
        "订什么座 [商(务座), 一(等座), 二(等座), 高(级软卧), 软卧, 硬卧, 动(卧), 软座, 硬座, 无(座)]？")
    座位s = 问座位()

    浏览器.get(链接)

    说("INFO: 正在尝试进入订票界面")

    已测试次数 = 0
    # Try indefinitely until the portal is open
    while 我爱你:
        已测试次数 += 1
        说(f"INFO: 第 {已测试次数} 次尝试")
        等到你出现("#t-list table tbody")
        try:
            浏览器.execute_script(找火车 + f'afterGetPage("{火车次}");')
        except 错误:
            等(友好延时)
            浏览器.refresh()
            continue
        break
    说("INFO: 已经进入订票界面")
    说("INFO: 请确认有没有二次登录")

    等到你出现("#normal_passenger_id li input")
    说("INFO: 正在选择乘客")
    for 编号, 乘客 in 统统检查(浏览器.find_elements_by_css_selector("#normal_passenger_id li label")):
        if 乘客.text in 乘客s:
            # JavaScript works better
            浏览器.execute_script(
                f'document.querySelectorAll("#normal_passenger_id li input")[{编号}].click();')
    说("INFO: 乘客选好了")

    等到你出现("select#seatType_1")
    说("INFO: 正在选择席别")
    try:
        浏览器.execute_script(选座 + f'chooseSeatType("{席别}");')
    except 错误:
        求输入("无此席别，请自选后继续")
    说("INFO: 席别选好了")

    等到你出现("#submitOrder_id")
    说("INFO: 正在提交选择")
    浏览器.execute_script('document.querySelector("#submitOrder_id").click();')

    等到你出现("#confirmDiv")
    说("INFO: 正在选座")

    if 类型(座位s) is 字典:
        # beds
        for 位置 in 座位s:
            浏览器.execute_script(选座选床 + f'checkBed("{位置}", {座位s[位置]})')
    else:
        # seats
        for 座位 in 座位s[0]:
            浏览器.execute_script(选座选床 + f'checkSeat(0, "{座位}");')
        for 座位 in 座位s[1]:
            浏览器.execute_script(选座选床 + f'checkSeat(1, "{座位}");')
    # Now click the final confirm
    等到你出现("#qr_submit_id")
    说("INFO: 正在提交订单")
    浏览器.execute_script(提交)
    说("INFO: 席位已锁定，请在30 分钟内进行支付")
    说("INFO: 请按任意键退出")
    求输入()
