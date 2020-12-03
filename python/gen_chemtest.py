#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

"""Generate Examcoo chemistry elements paper."""
import argparse

DATA = [['H', 'hydrogen', '氢'],
        ['He', 'helium', '氦'],
        ['Li', 'lithium', '锂'],
        ['Be', 'beryllium', '铍'],
        ['B', 'boron', '硼'],
        ['C', 'carbon', '碳'],
        ['N', 'nitrogen', '氮'],
        ['O', 'oxygen', '氧'],
        ['F', 'fluorine', '氟'],
        ['Ne', 'neon', '氖'],
        ['Na', 'sodium', '钠'],
        ['Mg', 'magnesium', '镁'],
        ['Al', 'aluminium|||aluminum', '铝'],
        ['Si', 'silicon', '硅'],
        ['P', 'phosphorus', '磷'],
        ['S', 'sulfur|||sulphur', '硫'],
        ['Cl', 'chlorine', '氯'],
        ['Ar', 'argon', '氩'],
        ['K', 'potassium', '钾'],
        ['Ca', 'calcium', '钙'],
        ['Sc', 'scandium', '钪'],
        ['Ti', 'titanium', '钛'],
        ['V', 'vanadium', '钒'],
        ['Cr', 'chromium', '铬'],
        ['Mn', 'manganese', '锰'],
        ['Fe', 'iron', '铁'],
        ['Co', 'cobalt', '钴'],
        ['Ni', 'nickel', '镍'],
        ['Cu', 'copper', '铜'],
        ['Zn', 'zinc', '锌'],
        ['Ga', 'gallium', '镓'],
        ['Ge', 'germanium', '锗'],
        ['As', 'arsenic', '砷'],
        ['Se', 'selenium', '硒'],
        ['Br', 'bromine', '溴'],
        ['Kr', 'krypton', '氪'],
        ['Rb', 'rubidium', '铷'],
        ['Sr', 'strontium', '锶'],
        ['Y', 'yttrium', '钇'],
        ['Zr', 'zirconium', '锆'],
        ['Nb', 'niobium', '铌'],
        ['Mo', 'molybdenum', '钼'],
        ['Tc', 'technetium', '锝'],
        ['Ru', 'ruthenium', '钌'],
        ['Bh', 'rhodium', '铑'],
        ['Pd', 'palladium', '钯'],
        ['Ag', 'silver', '银'],
        ['Cd', 'cadmium', '镉'],
        ['In', 'indium', '铟'],
        ['Sn', 'tin', '锡'],
        ['Sb', 'antimony', '锑'],
        ['Te', 'tellurium', '碲'],
        ['I', 'iodine', '碘'],
        ['Xe', 'xenon', '氙'],
        ['Cs', 'caesium', '铯'],
        ['Ba', 'barium', '钡'],
        ['La', 'lanthanum', '镧'],
        ['Ce', 'cerium', '铈'],
        ['Pr', 'praseodymium', '镨'],
        ['Nd', 'neodymium', '钕'],
        ['Pm', 'promethium', '钷'],
        ['Sm', 'samarium', '钐'],
        ['Eu', 'europium', '铕'],
        ['Gd', 'gadolinium', '钆'],
        ['Tb', 'terbium', '铽'],
        ['Dy', 'dysprosium', '镝'],
        ['Ho', 'holmium', '钬'],
        ['Er', 'erbium', '铒'],
        ['Tm', 'thulium', '铥'],
        ['Yb', 'ytterbium', '镱'],
        ['Lu', 'lutetium', '镥'],
        ['Hf', 'hafnium', '蛤'],
        ['Ta', 'tantalum', '钽'],
        ['W', 'tungsten', '钨'],
        ['Re', 'rhenium', '铼'],
        ['Os', 'osmium', '锇'],
        ['Ir', 'iridium', '铱'],
        ['Pt', 'platinum', '铂'],
        ['Au', 'gold', '金'],
        ['Hg', 'mercury', '汞'],
        ['Tl', 'thallium', '铊'],
        ['Pb', 'lead', '铅'],
        ['Bi', 'bismuth', '铋'],
        ['Po', 'polonium', '钋'],
        ['At', 'astatine', '砹'],
        ['Rn', 'radon', '氡'],
        ['Fr', 'francium', '钫'],
        ['Ra', 'radium', '镭'],
        ['Ac', 'actinium', '锕'],
        ['Th', 'thorium', '钍'],
        ['Pa', 'protactinium', '镤'],
        ['U', 'uranium', '铀'],
        ['Np', 'neptunium', '镎'],
        ['Pu', 'plutonium', '钚'],
        ['Am', 'americium', '镅'],
        ['Cm', 'curium', '锔'],
        ['Bk', 'berkelium', '锫'],
        ['Cf', 'californium', '锎'],
        ['Es', 'einsteinium', '锿'],
        ['Fm', 'fermium', '镄'],
        ['Md', 'mendelevium', '钔'],
        ['No', 'nobelium', '锘'],
        ['Lr', 'lawrencium', '铹'],
        ['Rf', 'rutherfordium', '𬬻|||钅卢|||鑪|||鈩|||lu'],
        ['Db', 'dubnium', '𨧀|||钅杜|||du'],
        ['Sg', 'seaborgium', '𬭳|||𨭎|||钅喜|||xi'],
        ['Bh', 'bohrium', '𬭛|||𨨏|||钅波|||bo'],
        ['Hs', 'hassium', '𬭶|||𨭆|||钅黑|||hei'],
        ['Mt', 'meitnerium', '鿏|||䥑|||钅麦|||mai'],
        ['Ds', 'darmstadtium', '𫟼|||鐽|||钅达|||da'],
        ['Rg', 'roentgenium', '𬬭|||錀|||钅仑|||lun'],
        ['Cn', 'copernicium', '鿔|||鎶|||钅哥|||ge'],
        ['Nh', 'nihonium', '鿭|||鑈|||鉨|||钅尔|||ni'],
        ['Fl', 'flerovium', '𫓧|||鈇|||钅夫|||fu'],
        ['Mc', 'moscovium', '镆|||鏌|||钅莫|||mo'],
        ['Lv', 'livermorium', '𫟷|||鉝|||钅立|||li'],
        ['Ts', 'tenassine', '鿬|||石田|||tian'],
        ['Og', 'oganesson', '鿫|||气奥|||奥气|||ao']]


def number2chinese(number: int):
    """Naive approach to convert number to chinese. Incorrect for numbers with zeroes in between."""
    res = []
    tbln = ["", "一", "二", "三", "四", "五", "六", "七", "八", "九"]
    tble = ["", "十", "百", "千", "万"]
    count = 0
    while True:
        rem = number % 10
        number //= 10
        res = [tbln[rem], tble[count]]+res
        count += 1
        if number == 0:
            break
    return ''.join(res)


def main():
    """Generate paper."""
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", type=int, default=1, help="分值")
    parser.add_argument("-t", type=str, default="化学元素多项记忆", help="标题")
    parser.add_argument("specs", type=str, default="ns,sc",
                        help="逗号分隔题型：s=符号,e=英文名,c=中文名,n=序数")
    namespace = parser.parse_args()
    title = f"试卷名称：{namespace.t}"
    score = f"分数:{namespace.s}"
    print(title, '\n')
    transform = {
        's': "元素符号",
        'e': "英文名",
        'c': "中文名",
        'n': "原子序数"
    }
    for nb, spec in enumerate(namespace.specs.split(',')):
        fromname = spec[0]
        toname = spec[1]
        print(
            f"{number2chinese(nb+1)}、{transform[fromname]}到{transform[toname]}\n")
        print("<TYPE.TAG>填空题\n")
        for n, a in enumerate(DATA):
            data = {
                's': a[0],
                'n': f"{n+1}",
                'e': a[1],
                'c': a[2]
            }
            print(
                f"{n+1}. {data[fromname]} = <FILL.TAG>{data[toname]}</FILL.TAG>（填{transform[toname]}）")
            print(score)
            print("")


if __name__ == "__main__":
    main()
