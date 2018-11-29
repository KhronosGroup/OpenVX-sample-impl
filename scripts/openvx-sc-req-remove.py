import sys
import os.path

dir_inc = 'include'
dir_spec = 'docs/specification'
flist = [ dir_inc + '/VX/vx.h',
		  dir_inc + '/VX/vx_api.h',
		  dir_inc + '/VX/vx_compatibility.h',
		  dir_inc + '/VX/vx_kernels.h',
		  dir_inc + '/VX/vx_nodes.h',
		  dir_inc + '/VX/vx_types.h',
		  dir_inc + '/VX/vx_vendors.h',
		  dir_inc + '/VX/vxu.h',
		  dir_spec + '/00_introduction.dox',
		  dir_spec + '/01_design_overview.dox',
		  dir_spec + '/02_modules.dox' ]
cfile = 'scripts/openvx-sc-req-count.txt'

if len(sys.argv) < 2:
	print('Usage: python scripts/openvx-sc-req-remove.py all|file(s)')
	print('   list of useful files:')
	for f in flist:
		print('     ' + f)
	sys.exit(1)

if os.path.exists(cfile):
	os.remove(cfile)

for fname in flist:
	fp = open(fname, 'r')
	ilines = fp.readlines()
	fp.close()
	olines = []
	rcount = 0
	for line in ilines:
		items = line.split(' [*R')
		nreq = len(items) - 1
		if nreq > 0:
			line = items[0]
			for item in items[1:]:
				rcount = rcount + 1
				line = line + ']'.join(item.split(']')[1:])
		items = line.split('[*R')
		nreq = len(items) - 1
		if nreq > 0:
			line = items[0]
			for item in items[1:]:
				rcount = rcount + 1
				line = line + ']'.join(item.split(']')[1:])
		olines.append(line)
	print('OK: removed %d requirements in %s' % (rcount, fname))
	fp = open(fname, 'w')
	for line in olines:
		fp.write(line)
	fp.close()
