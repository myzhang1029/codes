import numpy as np
from scipy.io import wavfile

data = wavfile.read("/Users/zmy/Downloads/untitled.wav")[1]


def zero_crossings(buf: np.ndarray) -> list[int]:
    THRESHOLD = 0.01
    positive = False
    result = []
    for n, dat in enumerate(buf):
        if not positive and dat > THRESHOLD:
            positive = True
            result.append(n)
        elif positive and dat < -THRESHOLD:
            positive = False
            result.append(n)
    return result


def get_track(bitstring: str) -> int:
    t1_percent = bitstring.find("1010001")
    t1_question = bitstring.find("1111100")
    if t1_percent > -1 and t1_question > t1_percent:
        return 1
    t2_semicol = bitstring.find("11010")
    t2_question = bitstring.find("11111")
    if t2_semicol > -1 and t2_question > t2_semicol:
        return 2
    return 0


class AverageArray:
    def __init__(self, size: int = 5) -> None:
        self.size = size
        self.data = []

    def add(self, new: int):
        self.data = self.data[:self.size-1] + [new]

    def average(self) -> float:
        return np.average(self.data)


def decode(zcros: list[int]):
    TRACK1_SIZE = 7
    TRACK1_ASCII = 32
    TRACK2_SIZE = 5
    TRACK2_ASCII = 48
    decoded = ""
    unknown = 0
    zeros = []
    ones = []
    unknowns = []
    avgarr = AverageArray()
    for i in range(1, len(zcros)-1):
        avg = avgarr.average()
        base = zcros[i-1]
        t = zcros[i] - base
        t2 = zcros[i+1] - base
        if i < 10:
            if i > 5:
                avgarr.add(t)
            continue
        if abs(t - avg) < avg * 0.3:
            decoded += "0"
            avgarr.add(t)
            zeros.append(zcros[i])
        elif abs(t2 - avg) < avg * 0.3:
            decoded += "1"
            avgarr.add(t2)
            ones.append(zcros[i])
        else:
            unknown += 1
            unknowns.append(zcros[i])
    start = decoded.find("1")
    end = decoded.rfind("1")
    decoded = decoded[end:start-1:-1]
    track = get_track(decoded)
    if not track:
        track = get_track(decoded[::-1])
    if not track:
        return None
    return track

print(zero_crossings(data))
print(decode(zero_crossings(data)))
