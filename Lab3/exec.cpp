#include<iostream>
using namespace std;
int main(int argc, char *args[]){
	system("gnome-terminal -- bash -c './main.out 8000 8001 5; exec bash'");
	system("gnome-terminal -- bash -c './main.out 8001 8002 5; exec bash'");
	system("gnome-terminal -- bash -c './main.out 8002 8000 5; exec bash'");
}
