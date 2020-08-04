#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#
#  extract_links.py
#  Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>
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

"""Extract video play pages from QQ Video detail pages."""

import sys
from typing import List
from selenium.webdriver import Edge


def get_play_links(detail_page: str) -> List[str]:
    """Get play links on a detail page."""
    drv = Edge()
    # drv.get("https://v.qq.com/detail/d/dxd1v76tmu0wjuj.html")
    # drv.get("https://v.qq.com/detail/s/sx5xljydk45g1pp.html")
    drv.get(detail_page)
    elem = drv.find_element_by_xpath(
        "/html/body/div[2]/div[2]/div[1]/div/div[1]/div[1]/span/div/div/div")

    # Expand if collapsed
    # Get the last button
    expand_button = elem.find_elements_by_tag_name("a")[-1]
    target_div = "/html/body/div[2]/div[2]/div[1]/div/div[1]/div[1]/span/div/div/div"
    if expand_button.get_property("data_range"):
        # The last button is an expand button
        expand_button.click()
        target_div = "/html/body/div[2]/div[2]/div[1]/div/div[1]/div[1]/span/div/div/div[2]"

    hrefdiv = drv.find_element_by_xpath(target_div)
    hrefs = [a.get_property("href")
             for a in hrefdiv.find_elements_by_tag_name("a")]
    drv.close()
    drv.quit()
    return hrefs


if __name__ == "__main__":
    for n, href in enumerate(get_play_links(sys.argv[1])):
        print(1+n, href)
