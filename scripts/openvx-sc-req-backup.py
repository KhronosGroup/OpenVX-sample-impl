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
	print('Usage: python scripts/openvx-sc-req-backup.py <folder>')
	print('   list of files to backup:')
	for f in flist:
		print('     ' + f)
	sys.exit(1)

folder = sys.argv[1]

if not os.path.exists(folder):
	os.mkdir(folder)

for ifile in flist:
	ofile = folder + '/' + ifile.split('/')[-1]
	if os.path.exists(ifile):
		print('OK: copying %s into %s' % (ifile, ofile))
		fp = open(ifile, 'r')
		lines = fp.readlines()
		fp.close()
		fp = open(ofile, 'w')
		for line in lines:
			fp.write(line)
		fp.close()
