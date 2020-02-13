#!/usr/bin/env python3

import readline
import sys
import argparse
import jieba


class pyDictionary:
    """NLP dictionary for use only in this program.
    There are two dictionaries:
    user dictionary: Read only, provided by the user.
    python dictionary: Read/Write, program generated.
    """

    def __init__(self, filename, pyfilename):
        self.filename = filename
        self.pyfilename = pyfilename
        self.dict = set(
            line.split(' ')[0].strip()
            for line in open(filename).readlines()
        )
        try:
            self.dict |= set(
                line.split(' ')[0].strip()
                for line in open(pyfilename).readlines()
            )
        except FileNotFoundError:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.save()

    def save(self):
        pydict = open(self.pyfilename, "w")
        pydict.write('\n'.join(self.dict))
        pydict.close()

    def add_words(self, to_add):
        self.dict |= set(to_add)

    def del_words(self, to_del):
        self.dict -= set(to_del)


class Corrector:
    """Base class for all WSs' correctors."""

    def __init__(
            self,
            inpfilename,
            outfilename,
            dictfilename="dict.txt",
            pydictfilename="pydict.txt"
    ):
        self.inpfilename = inpfilename
        self.outfilename = outfilename
        self.i = open(inpfilename, "r")
        self.o = open(outfilename, "a")
        self.dictfilename = dictfilename
        self.pydictfilename = pydictfilename
        self.dict = pyDictionary(dictfilename, pydictfilename)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def close(self):
        """Close all open files and save the dictionary."""
        self.i.close()
        self.o.close()
        self.dict.save()

    def add_words(self, words):
        """Add words, chiled classes may overwrite this method."""
        self.dict.add_words(words)

    def del_words(self, words):
        """Delete words, chiled classes may overwrite this method."""
        self.dict.del_words(words)

    def cut(self, line):
        """Default dummy cutter, Implements a maximum forward match.
        Unknown words become single characters."""
        result = ''
        # character before nmf are skipped
        nextmeaningful = 0
        # n: current char
        for n, c in enumerate(line):
            word = ''
            if n < nextmeaningful:
                continue
            # l: length of current word trying
            for l in range(1, len(line) - n + 1):
                # Current characters
                maybe_word = line[n:l+n]
                # Test if this combination is a word
                if maybe_word in self.dict.dict:
                    # Overwrite each time it finds a longer word
                    word = maybe_word
                    nextmeaningful = n + l
            if word:
                result += word + ' '
            else:
                result += c + ' '
                nextmeaningful += 1
        return result[:-1]  # remove trailing ws

    def lines(self):
        """Generator that goes over all lines."""
        # Using a generator instead of a AIO read
        # to save memory in case the data is huge
        while True:
            line = self.i.readline()
            if line:
                yield line.strip()
            else:
                return None

    def correct(self):
        """Correct NLP cut data for exporting train data."""
        # State file, save the progress(based on inpfile)
        # This works only given that the total number of lines
        # in self.inpbuffer never changes
        state_name = self.outfilename + ".crrst"
        try:
            # state is the idx `TO BE corrected
            state = int(open(state_name, "r").read())
        except FileNotFoundError:
            # Brand new correction
            state = 0
        with open(state_name, "w") as state_file:
            for n, line in enumerate(self.lines()):
                # Jump over corrected lines
                if n < state:
                    continue
                line = self.cut(line)
                # Write to state file every time to prevent data loss
                # Put this here in case one stops before processing any line.
                state_file.seek(0)
                state_file.write(str(n))
                print(f"\n\n{n}: {line}\n")
                readline.add_history(line)
                print("Is this correct? Please correct or leave empty. "
                      "Use space to delete this line. "
                      "Push Up to retrieve original entry.")
                # Readline enabled.
                newline = input("> ")
                # delete line
                if newline.isspace():
                    continue
                # Discover new words
                words = set((w for w in newline.split(' ') if w))
                print(f"Words: {words if words else '{}'}")
                # Add these new words to dictionary
                self.dict.add_words(words)
                # Remove any extra whitespace
                newline = newline.strip()
                # Replace the line if the user has one
                if newline:
                    self.o.write(newline + '\n')
                else:
                    self.o.write(line + '\n')


class JiebaCorrector(Corrector):
    """Correct jieba cut result."""

    def __init__(
            self,
            inpfilename,
            outfilename,
            dictfilename="dict.txt",
            pydictfilename="pydict.txt"
    ):
        super().__init__(
            inpfilename,
            outfilename,
            dictfilename,
            pydictfilename
        )
        self.jieba = jieba.Tokenizer()
        self.jieba.load_userdict(dictfilename)
        self.jieba.load_userdict(pydictfilename)

    def del_words(self, words):
        super().del_words(words)
        for w in words:
            self.jieba.del_word(w)

    def cut(self, line):
        self.dict.save()
        self.jieba.load_userdict(self.pydictfilename)
        return ' '.join(self.jieba.cut(line))


def main(corr=JiebaCorrector):
    ap = argparse.ArgumentParser()
    ap.add_argument("inputfile", type=str)
    ap.add_argument("outputfile", type=str)
    ap.add_argument("-d", "--dict", type=str, default="dict.txt")
    ap.add_argument("-p", "--pydict", type=str, default="pydict.txt")
    ap.add_argument("-r", "--delete", type=str, nargs='*')
    ns = ap.parse_args()
    with corr(
        ns.inputfile,
        ns.outputfile,
        ns.dict,
        ns.pydict
    ) as corrector:
        if ns.delete:
            corrector.del_words(ns.delete)
        corrector.correct()

if __name__ == "__main__":
    main()
