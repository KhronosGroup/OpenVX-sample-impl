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
	print('Usage: python scripts/openvx-sc-req-update.py [-reset-req-count|-revert-req-ids] all|file(s)')
	print('   list of useful files:')
	for f in flist:
		print('     ' + f)
	sys.exit(1)

revertMode = False
count = 0
if os.path.exists(cfile):
	fp = open(cfile, 'r')
	count = int(fp.readline().strip())
	fp.close()
	print('OK: pre-assigned requirement count is %d' % (count))

fileList = sys.argv[1:]
if sys.argv[1] == '-reset-req-count':
	count = 0
	fileList = sys.argv[2:]
elif sys.argv[1] == '-revert-req-ids':
	count = 0
	revertMode = True
	fileList = sys.argv[2:]

if (len(fileList) == 0) or (fileList[0] == 'all'):
	fileList = flist

for fname in fileList:
	fp = open(fname, 'r')
	ilines = fp.readlines()
	fp.close()
	olines = []
	if revertMode:
		rcount = 0
		for line in ilines:
			items = line.split('[*R')
			nreq = len(items) - 1
			if nreq > 0:
				line = items[0]
				for item in items[1:]:
					rcount = rcount + 1
					line = line + '[*REQ*]' + ']'.join(item.split(']')[1:])
			olines.append(line)
		print('OK: reverted %d requirements in %s' % (rcount, fname))
	else:
		rcount = 0
		for line in ilines:
			items = line.split('[*REQ*]')
			nreq = len(items) - 1
			if nreq > 0:
				line = items[0]
				for item in items[1:]:
					rcount = rcount + 1
					count = count + 1
					line = line + '[*R%05d*]' % (count)
					line = line + item
			olines.append(line)
		print('OK: identified %d requirements in %s' % (rcount, fname))
	fp = open(fname, 'w')
	for line in olines:
		fp.write(line)
	fp.close()

print('OK: total requirement count after update is %d' % (count))
fp = open(cfile, 'w')
fp.write('%d\n' % (count))
fp.close()
