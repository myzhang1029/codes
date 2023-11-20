#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#
#  id_watermark.py
#
#  Copyright (C) 2023 Zhang Maiyun <me@myzhangll.xyz>
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

"""身份证照片加水印。"""

from random import randint as 随机数

from PIL import ImageFont as 字体
from PIL import ImageDraw as 绘制
from PIL import Image as 图片

def 制水印(原图: 图片, 用途: str) -> 图片:
    """制作加了水印的图片。"""
    # 去除exif
    新图 = 图片.new(原图.mode, 原图.size)
    新图.putdata(原图.getdata())
    水印文字 = f"仅供{用途}，挪用无效"
    字体名 = "楷体_GB2312.ttf"
    字号 = 1
    水印字体 = 字体.truetype(字体名, 字号)
    图宽, 图高 = 新图.size
    文字宽 = 图宽 * 0.2
    # 确定合适的字号
    while 水印字体.getbbox(水印文字)[2] < 文字宽:
        字号 += 1
        水印字体 = 字体.truetype(字体名, 字号)
    # 减一
    字号 -= 1
    水印字体 = 字体.truetype(字体名, 字号)
    水印宽, 水印高 = 水印字体.getbbox(水印文字)[2:]
    print(f"字号为{字号}")
    # 重复水印文字
    for x in range(0, 图宽, 水印宽):
        for y in range(0, 图高, 2 * 水印高):
            # 随机颜色
            颜色最高 = 0x5f
            颜色 = (
                随机数(0, 颜色最高),
                随机数(0, 颜色最高),
                随机数(0, 颜色最高),
                130,
            )
            水印图片 = 图片.new("RGBA", (水印宽, 水印高), (0, 0, 0, 0))
            水印绘制 = 绘制.Draw(水印图片)
            水印绘制.text((0, 0), 水印文字, font=水印字体, fill=颜色)
            # 随机旋转
            倾斜角度 = 20 + 随机数(-1, 1) / 2
            水印图片 = 水印图片.rotate(倾斜角度, expand=1)
            # 随机坐标
            随机率 = 5
            坐标 = (
                x + 随机数(0, 水印宽 // 随机率),
                y + 随机数(0, 水印高 // 随机率),
            )
            新图.paste(水印图片, 坐标, 水印图片)
    新图.thumbnail((图宽, 图高), 图片.BILINEAR)
    return 新图

if __name__ == '__main__':
    from sys import argv as 参数
    from sys import exit as 退出
    if len(参数) != 4:
        print("用法：id_watermark.py <原图> <用途> <输出文件>")
        退出(1)
    原图 = 图片.open(参数[1])
    用途 = 参数[2]
    输出文件 = 参数[3]
    制水印(原图, 用途).save(输出文件)

