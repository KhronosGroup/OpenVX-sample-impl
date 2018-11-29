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
		  dir_spec + '/02_modules.dox',
		  'scripts/openvx-sc-req-count.txt' ]

if len(sys.argv) < 2:
	print('Usage: python scripts/openvx-sc-req-restore.py <folder>')
	print('   list of files to restore:')
	for f in flist:
		print('     ' + f)
	sys.exit(1)

folder = sys.argv[1]

if not os.path.exists(folder):
	print('ERROR: unable to find folder: ' + folder)
	sys.exit(1)

for ifile in flist:
	ofile = folder + '/' + ifile.split('/')[-1]
	if os.path.exists(ofile):
		print('OK: copying %s into %s' % (ofile, ifile))
		fp = open(ofile, 'r')
		lines = fp.readlines()
		fp.close()
		fp = open(ifile, 'w')
		for line in lines:
			fp.write(line)
		fp.close()
