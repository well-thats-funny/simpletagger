#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import pathlib
import uuid

import cbor2

class TagLibrary:
    top_level_keys = {
        1: 'format_version',
        2: 'app',
        3: 'root_node'
    }

    node_keys = {
        1: 'type',
        2: 'children',
        3: 'name',
        4: 'icon',
        5: 'uuid',
        6: 'link_to',
        7: 'tags',
        8: 'comment'
    }

    node_types = {
        1: 'root',
        2: 'collection',
        3: 'object',
        4: 'link'
    }

    @classmethod
    def process(cls, tree):
        tree = {cls.top_level_keys.get(k, k): v for k, v in tree.items()}

        def process(node):
            node = {k: v for k, v in sorted(node.items(), key=lambda i: i[0])}
            node = {cls.node_keys.get(k, k): v for k, v in node.items()}
            if 'type' in node:
                node['type'] = cls.node_types.get(node['type'], node['type'])
            if 'children' in node:
                node['children'] = [process(c) for c in node.pop('children')]
            return node

        tree['root_node'] = process(tree['root_node'])
        return tree


class Project:
    top_level_keys = {
        1: 'format_version',
        2: 'app',
        3: 'directories',
        4: 'excluded_files'
    }

    @classmethod
    def process(cls, tree):
        tree = {cls.top_level_keys.get(k, k): v for k, v in tree.items()}
        return tree


class FileTags:
    top_level_keys = {
        1: 'format_version',
        2: 'app',
        3: 'tags',
        4: 'region',
        5: 'tags_wizard_data'
    }

    @classmethod
    def process(cls, tree):
        tree = {cls.top_level_keys.get(k, k): v for k, v in tree.items()}
        return tree

formats = {'autodetect': None, 'taglibrary': TagLibrary, 'project': Project, 'filetags': FileTags}

parser = argparse.ArgumentParser()
parser.add_argument('FILE', type=pathlib.Path)
parser.add_argument('--format', choices=formats.keys(), default='autodetect')
args = parser.parse_args()

assert args.format != 'autodetect', 'autodetect not yet implemented'
format = formats[args.format]

with (args.FILE.open('rb') as f):
    tree = cbor2.load(f)

    tree = format.process(tree)

    INDENT           = r'    '
    INDENT_SUB_HEAD  = r'|---'
    INDENT_SUB_TAIL  = r'|   '
    INDENT_SUB_LAST  = r'\---'

    def dump(value, name=None, head_prefix='', tail_prefix=''):
        if isinstance(value, (int, str, bytes)):
            if name in ('uuid', 'link_to'):
                value = uuid.UUID(bytes=value)

            if name is not None:
                print(f'{head_prefix}{name}: {value!r}')
            else:
                print(f'{head_prefix}{value!r}')
        elif isinstance(value, list):
            if name is not None:
                print(f'{head_prefix}{name}:')
                dump(value, None, tail_prefix, tail_prefix)
            else:
                for i, v in enumerate(value):
                    if i < len(value) - 1:
                        sub_head_prefix = tail_prefix + INDENT_SUB_HEAD
                        sub_tail_prefix = tail_prefix + INDENT_SUB_TAIL
                    else:
                        sub_head_prefix = tail_prefix + INDENT_SUB_LAST
                        sub_tail_prefix = tail_prefix + INDENT
                    dump(v, None, sub_head_prefix, sub_tail_prefix)
        elif isinstance(value, dict):
            if name is not None:
                print(f'{head_prefix}{name}:')
                dump(value, None, tail_prefix, tail_prefix)
            else:
                sub_head_prefix = head_prefix
                sub_tail_prefix = tail_prefix
                for i, (k, v) in enumerate(value.items()):
                    if i == 0:
                        _head_prefix = sub_head_prefix
                        _tail_prefix = sub_tail_prefix
                    else:
                        _head_prefix = sub_tail_prefix
                        _tail_prefix = sub_tail_prefix

                    dump(v, k, _head_prefix, _tail_prefix)
        else:
            assert False, (value, type(value), name)

    dump(tree)
