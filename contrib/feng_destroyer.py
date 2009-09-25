#!/usr/bin/python
import os, sys, signal, random
from datetime import datetime, timedelta

VLC_POOL_SIZE = 10 #number of vlc client to keep open
SPAWN_TIMER = 5 #number of seconds between vlc spawn
MAX_VLC_LIFE_TIME = 60 #max life time in seconds of a vlc client
VLC_COMMAND = '/usr/bin/vlc'

class Vlc(object):
	def __init__(self, uri):
		super(Vlc, self).__init__()
		self.pid = None
		self.uri = uri
		self.spawn_time = None

	def _close_all_open_fd(self):
	    for fd in xrange(0, os.sysconf('SC_OPEN_MAX')): 
		try:
		    os.close(fd) 
		except OSError: 
		    pass 
		

	def run(self):
		if self.pid:
			return False

		pid = os.fork()
		if pid:
			self.pid = pid
			self.spawn_time = datetime.now()
			return True
		else:
			self._close_all_open_fd()
			os.execvp(VLC_COMMAND, ['vlc', self.uri])
			return None

	def stop(self):
		if not self.pid:
			return False

		try:
			os.kill(self.pid, signal.SIGTERM)
		        os.waitpid(self.pid, 0)
		except Exception, e:
			print 'Vlc wasn\'t here anymore', e
			pass

		return True

def main(url):
	random.seed()
	last_spawn = datetime.now() - timedelta(0, SPAWN_TIMER)
	vlc_pool = []

	while True:
		to_remove = []
		now = datetime.now()

		if (now - last_spawn >= timedelta(0, SPAWN_TIMER)) and (len(vlc_pool) < VLC_POOL_SIZE):
			last_spawn = now
			vlc = Vlc(url)
			print 'Running a new vlc'
			state = vlc.run()
			if state:
				vlc_pool.append(vlc)
			elif state == None:
				print 'Vlc Client exited by itself?'
				return
			else:
				print 'Failed to start Vlc'

		for vlc in vlc_pool:
			if now - vlc.spawn_time >= timedelta(0, MAX_VLC_LIFE_TIME):
				if random.random() >= 0.5:
					print 'Stopping an old vlc started at', vlc.spawn_time
					vlc.stop()
					to_remove.append(vlc)

		if len(to_remove) and random.random() > 0.95:
			for vlc in vlc_pool:
				if not vlc in to_remove:
					print 'Stopping multiple vlcs', vlc.spawn_time
					vlc.stop()
					to_remove.append(vlc)
					

		for vlc in to_remove:
			vlc_pool.remove(vlc)

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print '%s requires an rtsp url to request' % sys.argv[0]
	else:
		main(sys.argv[1])
