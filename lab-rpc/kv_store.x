typedef char buf <>;

struct put_args {
	buf k;
	buf v;
};

 program KVSTORE {
	version KVSTORE_V1 {
		int EXAMPLE(int) = 1;
		string ECHO(string) = 2;
		void PUT(put_args) = 3;
		buf GET(buf) = 4;
	} = 1;
} = 0x20000001;

