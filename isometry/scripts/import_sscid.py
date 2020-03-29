import psycopg, glob

conn = psycopg.connect(user = 'subspace', password = '<redacted>', database = 'subspace')
cursor = conn.cursor()

for filename in glob.glob('/tmp/ssc/*'):
	f = open(filename, 'r')
	data = psycopg.Binary(f.read())
	f.close()

	port = int(filename[filename.find('_')+1:filename.find('.')])
	cursor.execute('insert into isometry.sscid (ipaddress, port, sscid) values (inet \'75.101.147.52\', %d, %s)' % (port, data))

conn.commit()
cursor.close()
conn.close()
