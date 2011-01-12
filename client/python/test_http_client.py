# -*- coding:utf-8 -*-
from lib.httpsqs_client import httpsqs

sqs = httpsqs('10.249.200.168', '1218', 'test')

def test_put():
    l = ['a','b', 'c', 1, 2, 3]
    for node in l:
        print sqs.put(node)
        print sqs.put(node, 'other')

def test_get():
    print sqs.get()
    print sqs.get('other')
def test_gets():
    print sqs.gets()
    print sqs.gets('other')


def test_status():
    print sqs.status()
    print sqs.status('other')

def test_view(pos):
    print sqs.view(pos)
    print sqs.view(pos,'other')

def test_status_json():
    print sqs.status_json()
    print sqs.status_json('other')
    
def test_reset():
    print sqs.reset()
    print sqs.reset('other')

if __name__ == '__main__':
#    test_put()
#    test_get()
#    test_status()
#    test_view(20)
    test_status_json()
#    test_gets()
    test_reset()
    