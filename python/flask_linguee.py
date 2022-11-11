#!/usr/bin/env python3
#
#  flask_linguee.py
#
#  Copyright (C) 2021 Zhang Maiyun <me@myzhangll.xyz>
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


"""Flask web sevice to simplify checking English or French word on Linguee."""
import bs4
import flask
import requests

URL = "https://www.linguee.com/english-french/translation/{}.html"


def check(word: str) -> str:
    """Check English or French word on Linguee."""
    real_url = URL.format(word)
    template = """<html>
    <head>
        <style>
            div.isForeignTerm, div.isMainTerm {{
                display: block !important;
            }}
            h2 {{
                font-size: 1em;
            }}
            h3 {{
                font-size: 0.8em;
            }}
            .row_container {{
                display: flex;
                flex-direction: row;
                flex-wrap: wrap;
                width: 100%;
            }}
            .column_container {{
                display: flex;
                flex-direction: column;
                flex-basis: 100%;
                flex: 1;
            }}
        </style>
    </head>
    <body>
        <div class="row_container">
            <div class="column_container"><h1>English to French</h1>{}</div>
            <div class="column_container"><h1>French to English</h1>{}</div>
        </div>
    </body>
</html>
"""
    res = requests.get(real_url)
    res.raise_for_status()
    soup = bs4.BeautifulSoup(res.content, features="html.parser")
    french_english = soup.select("#dictionary > div.isForeignTerm")
    fr_en_str = ''.join([str(x) for x in french_english])
    english_french = soup.select("#dictionary > div.isMainTerm")
    en_fr_str = ''.join([str(x) for x in english_french])
    return template.format(en_fr_str, fr_en_str)


app = flask.Flask(__name__)


@app.route("/", methods=['GET', 'POST'])
def searcher():
    """Flask app."""
    query = flask.request.args.get('q')
    return check(query)


if __name__ == "__main__":
    app.run()
