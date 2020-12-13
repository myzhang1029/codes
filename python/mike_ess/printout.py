#!/usr/bin/env python3

"""Receive data."""
import sys


def toi(string, si, ei, sf=1):
    """Slice string, convert to int and scale."""
    if string[si:ei].strip() == -9999:
        return ""
    return str(int(string[si:ei])/sf)


def incr_date(date: str):
    """Increasea date of the fore YYYYMMDD."""
    # Months with 31 days
    mo32 = {1, 3, 5, 7, 8, 10, 12}
    yr = int(date[0:4])
    mo = int(date[4:6])
    dy = int(date[6:8])
    dy += 1
    if mo == 2 and yr == 2016 and dy > 29:
        dy -= 1
        dy %= 29
        dy += 1
        mo += 1
    elif mo == 2 and dy > 28:
        dy -= 1
        dy %= 28
        dy += 1
        mo += 1
    elif mo in mo32 and dy > 31:
        dy -= 1
        dy %= 31
        dy += 1
        mo += 1
    elif mo not in mo32 and dy > 30:
        dy -= 1
        dy %= 30
        dy += 1
        mo += 1
    if mo > 12:
        mo -= 1
        mo %= 12
        mo += 1
        yr += 1
    return f"{yr}{mo:02}{dy:02}"


def main_printout():
    """Print out data."""
    isd2015 = open("china_isd_lite_2015/545110-99999-2015").readlines()
    isd2016 = open("china_isd_lite_2016/545110-99999-2016").readlines()
    isd2017 = open("china_isd_lite_2017/545110-99999-2017").readlines()
    isd2018 = open("china_isd_lite_2018/545110-99999-2018").readlines()
    isd2019 = open("china_isd_lite_2019/545110-99999-2019").readlines()

    print("date/YYYYMMDD,time/HH,at/C,slp/hPa,wdNcw/deg,ws/ms-1,"
          "dongsiPM2.5,nongzhanguanPM2.5")
    for ln in isd2015+isd2016+isd2017+isd2018+isd2019:
        mo = int(ln[5:7])
        if mo not in {1, 11, 12}:
            continue
        if int(ln[25:31]) == -9999:
            continue
        nongzhanguan_idx = -1
        dongsi_idx = -1
        pm25_1 = ""
        pm25_2 = ""
        date_bj = f"{int(ln[0:4])}{int(ln[5:7]):02}{int(ln[8:11]):02}"
        time_bj = int(ln[11:13]) + 8
        if time_bj > 23:
            date_bj = incr_date(date_bj)
            time_bj %= 24
        pm25filename = (f"beijing_{date_bj[0:4]}0101-{date_bj[0:4]}1231/"
                        f"beijing_all_{date_bj}.csv")
        pm25file = open(pm25filename).readlines()
        if len(pm25file) == 0:
            print(
                f"Warning: empty pm25file on "
                f"{int(ln[0:4])}/{int(ln[5:7])}/{int(ln[8:11])}", file=sys.stderr)
        else:
            # Find index for dongsi and nongzhanguan
            for idx, name in enumerate(pm25file[0].split(',')):
                if name.strip() == "东四":
                    dongsi_idx = idx
                if name.strip() == "农展馆":
                    nongzhanguan_idx = idx
            if dongsi_idx < 0:
                raise ValueError("没有dongsi")
            if nongzhanguan_idx < 0:
                raise ValueError("没有nongzhanguan")
            for line in pm25file[1:]:
                values = line.strip().split(',')
                # Standarize to UTC time
                time = values[1]
                vtype = values[2].strip()
                if int(time) == time_bj and vtype == "PM2.5":
                    print(
                        f"Date {int(ln[0:4])}/{int(ln[5:7])}/{int(ln[8:11])} "
                        f"corresponds to file {pm25filename}", file=sys.stderr)
                    print(f"Time {int(ln[11:13])} is {time}", file=sys.stderr)
                    pm25_1 = str(float(values[dongsi_idx])
                                 ) if values[dongsi_idx] else ""
                    pm25_2 = str(float(values[nongzhanguan_idx]
                                       )) if values[nongzhanguan_idx] else ""
        print(f"{int(ln[0:4])}/{int(ln[5:7])}/{int(ln[8:11])},{int(ln[11:13])},"
              f"{toi(ln, 13, 19, 10)},{toi(ln, 25, 31, 10)},{toi(ln, 31, 37)},"
              f"{toi(ln, 37, 43, 10)},{pm25_1},{pm25_2}")


if __name__ == "__main__":
    main_printout()
