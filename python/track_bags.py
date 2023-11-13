#!/usr/bin/env python3
#
#  track_bags.py
#  Copyright (C) 2021 Zhang Maiyun <me@maiyun.me>
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

"""Track multiple bags with WorldTracer in a single webpage."""

import threading
from datetime import datetime
from flask import Flask
from selenium.webdriver import Firefox as WD
from selenium.webdriver import FirefoxOptions as WDOptions
from time import sleep
from typing import Dict

app = Flask(__name__)

# CONSTS
# Lastname to full names
NAMES: Dict[str, str] = {}
# Lastname to tracking numbers
FRS: Dict[str, str] = {}


def update_one(wd, last, fr, data):
    """Update DATA with all the packages with LAST as lastname ans FR as file ref."""
    wd.get("https://wtrweb.worldtracer.aero/WorldTracerWeb/pax.do?airlineCode=AC&siteLanguage=en")
    wd.find_element_by_xpath("//*[@id=\"record__reference\"]").send_keys(fr)
    wd.find_element_by_xpath(
        "//*[@id=\"skname__paxnameinternet\"]").send_keys(last)
    wd.find_element_by_xpath("//*[@id=\"btn_action\"]").click()
    bag_xpath = "/html/body/table/tbody/tr/td/div/table/tbody/tr[3]/td/table/tbody/tr/td/table/tbody/tr[4]/td/form/div/table/tbody/tr[{}]/td/table/tbody/tr"
    data[last] = [str(datetime.now()), *([ele.find_element_by_xpath("td[2]/div/strong").text for ele in wd.find_elements_by_xpath(
        bag_xpath.format(4))] + [ele.find_element_by_xpath("td[2]/div/strong").text for ele in wd.find_elements_by_xpath(bag_xpath.format(5))])]

def update(wd):
    """Update everyone in NAMES."""
    data = {}
    for last in NAMES.keys():
        update_one(wd, last, FRS[last], data)
    return data


wd_options = WDOptions()
wd_options.headless = True


@app.route("/")
def hello_world():
    global data
    response = "<br /><br />".join((f"""Name: {NAMES[l]}<br />
Last Update: {data[l][0]}<br />
Bags: <br />&nbsp;{'<br />&nbsp;'.join(data[l][1:])}
""" for l in NAMES.keys()))
    return response


def updater(wd):
    global data
    while True:
        data = update(wd)


# data = {}
with WD(options=wd_options) as wd:
    data = update(wd)
    thrd = threading.Thread(target=updater, args=(wd,))
    thrd.start()
    app.run(debug=False, use_reloader=False)
