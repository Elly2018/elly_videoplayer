

extern "C" {
	int interfaceCreatePlayer();
	void interfaceDestroyPlayer(int id);
	void interfacePlay(int id);
	void interfacePause(int id);
	void interfaceStop(int id);
	void interfaceSeek(int id, double time);
	double interfaceGetCurrentTime(int id);
	double interfaceMediaLength(int id);
	void interfaceLoadPath(int id, const char* path);
	void interfaceLoadPathAsync(int id, const char* path);
}