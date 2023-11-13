#!/usr/bin/env python3
#
#  down_edx.py
#
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

"""Automated and multi-threaded download of edX course videos - version 2."""

import argparse
import getpass
import json
import subprocess
import threading
import time
from typing import List, Optional

import selenium.webdriver as wd
from selenium.webdriver.common.by import By
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


def slow_type(element: WebElement, text: str, delay: float = 0.05):
    """Type into the input field slowly."""
    for char in text:
        element.send_keys(char)
        time.sleep(delay)


def down_video(name: str, url: str):
    """Download url into name."""
    while True:
        try:
            subprocess.run(
                ["aria2c", "-c", "-x16", "-o",
                    name if name[-4:] == ".mp4" else name + ".mp4", url],
                check=True)
        except subprocess.CalledProcessError:
            continue
        break


def main(url_file: str, account: Optional[str] = None,
         password: Optional[str] = None, timeout: Optional[int] = None):
    """Process a list of urls."""
    # Thread list
    threads: List[threading.Thread] = []
    # Element load timeout
    timeout = timeout or 10
    # Load Firefox webdriver
    ffoptions = wd.firefox.options.Options()
    ffoptions.headless = True
    drv = wd.Firefox(options=ffoptions)
    # Login
    drv.get("https://edx.org/login")
    # Wait for form to be loaded
    elem = EC.presence_of_element_located((By.ID, "login-password"))
    WebDriverWait(drv, timeout).until(elem)
    account = account or input("Account: ")
    password = password or getpass.getpass()
    slow_type(drv.find_element_by_id("login-email"), account)
    slow_type(drv.find_element_by_id("login-password"), password)
    drv.find_element_by_css_selector("button[type='submit']").click()
    while drv.current_url != "https://courses.edx.org/dashboard":
        time.sleep(0.1)
    pairs = [d.split()
             for d in open(url_file).readlines() if d and d[0] != '#']
    for (url, name) in pairs:
        video_getter(drv, url, name=name, timeout=timeout, threads=threads)
    drv.close()
    drv.quit()
    for thread in threads:
        thread.join()


def video_getter(drv, page_url: str, *, name: Optional[str] = None,
                 timeout: int, threads: List[threading.Thread]):
    """Get a video from a course page URL."""
    drv.get(page_url)
    # Wait for iframe to be loaded
    elem = EC.presence_of_element_located((By.ID, "unit-iframe"))
    WebDriverWait(drv, timeout).until(elem)
    # The iframe to the player
    iframe = drv.find_element_by_xpath(
        "//*[@id=\"unit-iframe\"]").get_attribute("src")
    drv.get(iframe)
    # Wait for video metadata to be loaded
    elem = EC.presence_of_element_located(
        (By.XPATH, "/html/body/div[4]/div/section/main/div[3]/div/div/div/div"))
    WebDriverWait(drv, timeout).until(elem)
    # Get video metadata
    div = drv.find_element_by_xpath(
        "/html/body/div[4]/div/section/main/div[3]/div/div/div/div").get_attribute("data-metadata")
    # Receive URL
    url: str = json.loads(div)["sources"][0]
    name = name or input(url + "\nname? ")
    if name == "":
        return
    # Download the video
    thread = threading.Thread(target=down_video, args=(name, url))
    threads.append(thread)
    thread.start()


def entry():
    """Entrypoint."""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--account", help="Account name", type=str)
    parser.add_argument("-p", "--password", help="Password", type=str)
    parser.add_argument("-t", "--timeout", help="Timeout", type=int)
    parser.add_argument("file", type=str, help="Download file,"
                        "the first column are URLs, and the second are names")
    args = parser.parse_args()
    main(args.file, account=args.account,
         password=args.password, timeout=args.timeout)


if __name__ == "__main__":
    entry()
