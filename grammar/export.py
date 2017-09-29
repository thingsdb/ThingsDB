#!/usr/bin/python3
'''Export python grammar to C grammar files

Author: Jeroen van der Heijden (Transceptor Technology)
Date: 2016-10-10
'''
import os
import sys
from rql import RqlLang


if __name__ == '__main__':
    rql = RqlLang()

    c_file, h_file = rql.export_c(target='lang/lang')

    EXPOTR_PATH = 'cgrammar'

    try:
        os.makedirs(EXPOTR_PATH)
    except FileExistsError:
        pass

    # with open(os.path.join(EXPOTR_PATH, 'lang.c'),
    #           'w',
    #           encoding='utf-8') as f:
    #     f.write(c_file)
    # with open(os.path.join(EXPOTR_PATH, 'lang.h'),
    #           'w',
    #           encoding='utf-8') as f:
    #     f.write(h_file)

    # Start overwrite project files
    with open(os.path.join('..', 'src', 'lang', 'lang.c'),
              'w',
              encoding='utf-8') as f:
        f.write(c_file)
    with open(os.path.join('..', 'inc', 'lang', 'lang.h'),
              'w',
              encoding='utf-8') as f:
        f.write(h_file)
    # End overwrite project files

    print('\nFinished creating new c-grammar files...\n')

    js_file = rql.export_js()

    EXPOTR_PATH = 'jsgrammar'

    try:
        os.makedirs(EXPOTR_PATH)
    except FileExistsError:
        pass

    with open(os.path.join(EXPOTR_PATH, 'grammar.js'),
              'w',
              encoding='utf-8') as f:
        f.write(js_file)

    print('\nFinished creating new js-grammar file...\n')

    py_file = rql.export_py()

    EXPOTR_PATH = 'pygrammar'

    try:
        os.makedirs(EXPOTR_PATH)
    except FileExistsError:
        pass

    with open(os.path.join(EXPOTR_PATH, 'grammar.py'),
              'w',
              encoding='utf-8') as f:
        f.write(py_file)

    print('\nFinished creating new py-grammar file...\n')

    go_file = rql.export_go()

    EXPOTR_PATH = 'gogrammar'

    try:
        os.makedirs(EXPOTR_PATH)
    except FileExistsError:
        pass

    with open(os.path.join(EXPOTR_PATH, 'grammar.go'),
              'w',
              encoding='utf-8') as f:
        f.write(go_file)

    print('\nFinished creating new go-grammar file...\n')