#!/usr/bin/env python3

"""A more versatile Python thekitd."""

import os
import subprocess
from math import e, log
from typing import Any, Dict, Tuple

import requests
from flask import Flask, redirect, request
from werkzeug.utils import secure_filename

UPLOAD_FOLDER = "/tmp"

DASHBOARD = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <meta charset="UTF-8" />
    <title> The Kit Controller </title>
    <style>
.color1 {background-color: #fffba9;}
.color2 {background-color: #eb80fd;}
.color3 {background-color: #76ff9a;}
.color4 {background-color: #fffbf4;}
.color5 {background-color: #ff817b;}

html {
    background-image: url("https://myzhangll.xyz/assets/img/scenery/image1.jpg");
    background-size: 100vw 100vh;
    text-align: center;
}

h1 {
    color: #411f07;
    font-family: fantasy;
    font-weight: bold;
    font-size: 1.5em;
}

#resblk {
    max-width: 100vw;
    text-align: start;
    word-wrap: break-word;
    overflow-x: auto;
}

button, .slider {
    color: #00152c;
    opacity: 0.7;
    margin: 0.5vh 1vw;
    border: 2px solid #bfe3ae;
    border-radius: 40px;
    font-family: sans-serif;
    font-size: 1.5em;
    height: 8vh;
}

.slider {
    -webkit-appearance: none;
    outline: none;
    -webkit-transition: .2s;
    transition: opacity .2s;
}

.slider:hover, button:hover, form:hover {
    opacity: 1;
}

.slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 8vh;
    height: 8vh;
    border-radius: 40px;
    background: #00152c;
    cursor: pointer;
}

.slider::-moz-range-thumb {
    width: 8vh;
    height: 8vh;
    border-radius: 40px;
    background: #00152c;
    cursor: pointer;
}

.reorg {
    display: grid;
    grid-template-columns: 50% 50%;
    width: 100%;
    vertical-align: middle;
}

form {
    margin: auto;
    font-size: 3em;
    opacity: 0.7;
}
    </style>
</head>
<body>
    <h1> The Kit Controller </h1>

    <div id="buttons">
        <!--div class=reorg>
            <button class="turnon color1" type="button" onclick="lightSwitch(1, true)">
                Big Mikey I
            </button>
            <button class="turnoff color1" type="button" onclick="lightSwitch(1, false)">
                Big Mikey O
            </button>
        </div-->
        <div class=reorg>
            <button class="turnon color2" type="button" onclick="lightSwitch(2, true)">
                Stephanie I
            </button>
            <button class="turnoff color2" type="button" onclick="lightSwitch(2, false)">
                Stephanie O
            </button>
        </div>
        <div class="reorg">
            <input type="range" min="0" max="100" value="50" class="slider color3" id="light3_dimmer" onchange="lightDim(3)">
            <!--button id="getinfo" class="color5" type="button" onclick="getInfo()">
                Mikey Info
            </button-->
        </div>
        <div class="reorg">
            <button id="playrecorded" class="color1" type="button" onclick="playRecorded()">
                Good Morning
            </button>
            <button id="playsent" class="color1" type="button" onclick="playSent()">
                Play Audio
            </button>
        </div>
        <form method="POST" enctype="multipart/form-data" action="/player">
            <input type="file" name="audio">
            <br />
            <input type="submit" value="Play at Mikey's">
        </form>
    </div>

    <pre id="resblk"></pre>

    <script>
        function xhrGlue(endpoint) {
            let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4) {
                    let text;
                    try {
                        const obj = JSON.parse(this.responseText);
                        text = JSON.stringify(obj, null, 2);
                    } catch (err) {
                        text = err.name + ": " + err.message;
                    }
                    document.getElementById("resblk").innerHTML = text;
                }
            };
            xhttp.open("GET", endpoint, true);
            xhttp.send();
        }
        function lightDim(n) {
            xhrGlue("/" + n + "light_dim?level=" + document.getElementById("light" + n + "_dimmer").value);
        }
        function lightSwitch(n, on) {
            xhrGlue("/" + n + "light_" + (on ? "on" : "off"));
        }
        function playRecorded() {
            xhrGlue("/playR");
        }
        function playSent() {
            xhrGlue("/playP");
        }
</script>

</body>
</html>"""


RH2 = "stephlight.local"
JSONResponse = Tuple[Dict[str, Any], int]


def _send_or(data: str, if_succeed: Dict[str, Any], remote_host: str) -> JSONResponse:
    """Send this data through or fail."""
    try:
        resp = requests.get("http://" + remote_host + "/" + data, timeout=5)
    except requests.exceptions.ConnectionError as exc:
        return {
            "success": False,
            "exception": str(exc),
        }, 500
    if resp.status_code == 200:
        return {**if_succeed, "message": resp.text}, 200
    return {
        "success": False,
        "error": resp.text,
        "error_code": resp.status_code,
    }, 500


def intensity_to_dcycle(intensity: float) -> int:
    real_intensity = e ** (intensity * log(101, e) / 100) - 1
    voltage = real_intensity * (19.2 - 7.845)/100 + 7.845
    if 7.845 < voltage <= 9.275:
        return int(281970 * (-7.664 + voltage))
    if 9.275 < voltage <= 13.75:
        return int(26520 * (6.959 + voltage))
    if 13.75 < voltage <= 16.88:
        return int(49485 * (-2.529 + voltage))
    if 16.88 < voltage <= 19.2:
        return min(int(21692 * (26.90 + voltage)), 1000000)
    return 0


app = Flask(__name__)
app.config["UPLOAD_FOLDER"] = UPLOAD_FOLDER


@app.route("/")
def _dashboard() -> Tuple[str, int]:
    return DASHBOARD, 200


@app.route("/2light_on")
def _2light_on() -> JSONResponse:
    return _send_or("on", {"light": True}, RH2)


@app.route("/2light_off")
def _2light_off() -> JSONResponse:
    return _send_or("off", {"light": False}, RH2)


@app.route("/3light_dim")
def _3light_dim() -> JSONResponse:
    value = request.args.get("level")
    light3_state = intensity_to_dcycle(float(value))
    open("/dev/thekit_pwm", "w").write(f"{light3_state}\n")
    return {"dim": True, "value": value}, 200


@app.route("/playR")
def _play_recorded() -> JSONResponse:
    return _send_or("play_rec", {"play": True}, RH2)


@app.route("/playP")
def _play_sent() -> JSONResponse:
    return _send_or("play_sent", {"play": True}, RH2)


@app.route("/player", methods=["POST"])
def play_music():
    if "audio" not in request.files:
        return {"error": "bad request", "reason": "no audio"}, 400
    audio = request.files["audio"]
    if not audio or not audio.filename:
        return {"error": "bad request", "reason": "audio is undefined"}, 400
    if audio.filename[-4:] in (".wav", ".m4a", ".mp4", ".aac"):
        filename = secure_filename(audio.filename)
        filename = os.path.join(app.config["UPLOAD_FOLDER"], filename)
        audio.save(filename)
        subprocess.run(
            ["ffplay", "-nodisp", "-autoexit", filename], check=True)
        os.remove(filename)
    return redirect("/")


app.run(host="0.0.0.0")
