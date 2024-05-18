#include <iostream>
#include <string>

using namespace std;

int main() {
	string line;
	char c;

	while ((c = cin.get()) != '\n') {
		if (c == '|') {
			for (size_t i = 0; i < 5; i++) {
				cin.putback(line.back());
				line.erase(line.end() - 1);
			}
		}

		cout << c;
		line.push_back(c);
	}

	cout << endl;

	return 0;
}