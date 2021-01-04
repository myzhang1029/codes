#!/usr/bin/env python3
# Released into the Public Domain

"""Automated download of edX course videos."""

import argparse
import getpass
import json
import subprocess
from typing import List, Optional

import selenium.webdriver as wd
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait


def main(urls: List[str], account: Optional[str] = None, password: Optional[str] = None, timeout: int = 10):
    """Process a list of urls."""
    # Load Firefox webdriver
    ffoptions = wd.firefox.options.Options()
    ffoptions.headless = True
    drv = wd.Firefox(options=ffoptions)
    # Login
    drv.get("https://edx.org/login")
    account = account or input("Account: ")
    password = password or getpass.getpass()
    drv.find_element_by_id("login-email").send_keys(account)
    drv.find_element_by_id("login-password").send_keys(password)
    drv.find_element_by_css_selector("button[type='submit']").click()
    for url in urls:
        video_getter(drv, url, timeout=timeout)
    drv.close()
    drv.quit()


def video_getter(drv, page_url: str, timeout: int):
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
    name = input(url + "\nname? ")
    if name == "":
        return
    # Download the video
    while True:
        try:
            subprocess.run(
                ["aria2c", "-x16", "-o", name if name[-4:] == ".mp4" else name + ".mp4", url], check=True)
        except subprocess.CalledProcessError:
            continue
        break


def entry():
    """Entrypoint."""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--account", help="Account name", type=str)
    parser.add_argument("-p", "--password", help="Password", type=str)
    parser.add_argument("-t", "--timeout", help="Timeout", type=int)
    parser.add_argument("urls", nargs="+")
    args = parser.parse_args()
    main(args.urls, account=args.account,
         password=args.password, timeout=args.timeout)


if __name__ == "__main__":
    entry()
