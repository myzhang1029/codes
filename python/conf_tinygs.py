#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  conf_tinygs.py
#
#  Copyright (C) 2024 Zhang Maiyun <maz005@ucsd.edu>
#   As a project developed during Maiyun's participation in the UCSD SRIP program,
#   The University of California may have claims to this work.
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


"""Configure TinyGS Gateways."""


import sys

from selenium.webdriver import Chrome
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as ecn
from selenium.webdriver.support.ui import Select, WebDriverWait

BASENAME = "AK6DS_sriptgs"
BASEPW = sys.argv[1]
WIFI_SSID = sys.argv[2]
WIFI_PSK = sys.argv[3]
MQTT_USER = sys.argv[4]
MQTT_PASS = sys.argv[5]
NUM = sys.argv[6]


with Chrome() as w:
    def waitfor(sel):
        return WebDriverWait(w, 100).until(ecn.visibility_of_element_located((By.XPATH, sel)))

    def typein(sel, text):
        elem = waitfor(sel)
        elem.clear()
        elem.send_keys(text)
    w.get("https://installer.tinygs.com/")
    input("Press Enter after finishing and connecting the device")
    w.get("http://192.168.4.1")
    waitfor("/html/body/div/button[2]").click()
    typein("//*[@id=\"iwcThingName\"]", BASENAME + NUM)
    typein("//*[@id=\"iwcApPassword\"]", BASEPW + NUM)
    typein("//*[@id=\"iwcWifiSsid\"]", WIFI_SSID)
    typein("//*[@id=\"iwcWifiPassword\"]", WIFI_PSK)
    typein("//*[@id=\"lat\"]", "32.882")
    typein("//*[@id=\"lng\"]", "-127.234")
    tz = Select(waitfor("//*[@id=\"tz\"]"))
    tz.select_by_visible_text("America/Los_Angeles")
    typein("//*[@id=\"mqtt_user\"]", MQTT_USER)
    typein("//*[@id=\"mqtt_pass\"]", MQTT_PASS)
    board = Select(waitfor("//*[@id=\"board\"]"))
    board.select_by_visible_text("433MHz LILYGO T3_V1.6.1")
    waitfor("//*[@id=\"tx\"]").click()
    input("Please confirm")
    waitfor("/html/body/div[2]/form/button").click()
    input("Press Enter to exit")
