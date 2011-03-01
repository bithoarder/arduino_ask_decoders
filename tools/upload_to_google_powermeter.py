#!/usr/bin/env python
import sys
import os
import couchdb
import itertools
import urllib2
import xml.etree.ElementTree as etree

couch = couchdb.Server('http://localhost:5984/')
db = couch['ask']

gpm_cfg = db['cfg']
token = gpm_cfg['token']
#hash = gpm_cfg['hash']
path = gpm_cfg['path']
last_key = gpm_cfg.get('last', '')

view = db.view('_design/couchapp/_view/cumulative', startkey=last_key, limit=1000, skip=1 if last_key else 0)
view = tuple(view)
if not view:
  print 'no new samples'
else:
  # todo: change to use element tree to generate the xml...
  xml = '''<?xml version="1.0" encoding="UTF-8"?>
  <feed xmlns="http://www.w3.org/2005/Atom"
        xmlns:meter="http://schemas.google.com/meter/2008"
        xmlns:batch="http://schemas.google.com/gdata/batch">
  '''

  for doc in view:
    xml += '''\
  <entry>
    <category scheme="http://schemas.google.com/g/2005#kind"
        term="http://schemas.google.com/meter/2008#instMeasurement"/>
    <meter:subject>
      https://www.google.com/powermeter/feeds%(path)s.c1
    </meter:subject>
    <meter:occurTime meter:uncertainty="2.0">%(time)s</meter:occurTime>
    <meter:quantity meter:uncertainty="0.001" meter:unit="kW h">%(usage).3f</meter:quantity>
  </entry>
  '''%dict(path=path, time=doc.key, usage=doc.value)
  
  xml += '</feed>'
  
  req = urllib2.Request('https://www.google.com/powermeter/feeds/event')
  req.add_header('Authorization', 'AuthSub token="%s"'%token)
  req.add_header('Content-Type', 'application/atom+xml')
  req.add_data(xml)
  f = urllib2.urlopen(req)
  reply = f.read()

  tree = etree.XML(reply)
  for entry in tree.findall('{http://www.w3.org/2005/Atom}entry'):
    key = entry.find('{http://schemas.google.com/meter/2008}occurTime').text
    status = int(entry.find('{http://schemas.google.com/gdata/batch}status').get('code'))
    if status != 201:
      print >>sys.stderr, '*** Error: batch upload failed (%s, %u)'%(key, status)
      print >>sys.stderr, reply
      sys.exit(1)

  gpm_cfg['last'] = view[-1].key
  db['cfg'] = gpm_cfg
  
  print 'uploaded %u samples'%len(view)

