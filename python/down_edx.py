#!/usr/bin/env python3
# Public domain

"""Automated download of edX course videos."""

import json
import subprocess
import sys
from typing import List, Dict

import selenium.webdriver as wd
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


def main(urls: List[str]):
    """Process a list of urls."""
    # Login cookies
    cookies: List[Dict[str, str]] = json.load(open("cookies.json"))
    # Load Firefox webdriver
    ffoptions = wd.firefox.options.Options()
    ffoptions.headless = True
    drv = wd.Firefox(options=ffoptions)
    # Load any URL to set cookie
    drv.get(urls[0])
    # Add login cookies
    for cookie in cookies:
        drv.add_cookie(cookie_dict=cookie)
    for url in urls:
        video_getter(drv, url)
    drv.close()
    drv.quit()


def video_getter(drv, page_url: str):
    """Get a video from a course page URL."""
    # Timeout for page to load
    timeout = 10
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
    name = input(url + "\nname? ")
    if name == "":
        return
    # Download the video
    subprocess.run(["aria2c", "-x16", "-o", name + ".mp4", url], check=True)


main(sys.argv[1:])
