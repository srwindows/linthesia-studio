#include "FolderOrdering.h"
#include "PersonalFolders.h"
#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
using std::string;
int main(){
 std::vector<string> order={"favorites","A","B","C","popular"};
 auto intact=order;
 assert(FolderOrdering::SwapPositions(order,"A","C"));
 assert((order==std::vector<string>{"favorites","C","B","A","popular"}));
 assert(!FolderOrdering::SwapPositions(order,"C","C"));
 assert(!FolderOrdering::SwapPositions(order,"missing","A"));
 assert(order[2]=="B"&&order[0]=="favorites"&&order[4]=="popular");
 char dir[]="/tmp/linthesia-twochoices-XXXXXX";
 char* tmp=mkdtemp(dir);assert(tmp);
 assert(setenv("XDG_DATA_HOME",tmp,1)==0);
 PersonalFolders personal;assert(personal.Ready());
 string a,b,error;
 assert(personal.Create(personal.Root(),"A",a,error));
 assert(personal.Create(personal.Root(),"B",b,error));
 {std::ofstream file(a+"/obra.mid",std::ios::binary);file<<"MThd-test-bytes";}
 {std::ofstream file(b+"/original.mid",std::ios::binary);file<<"MThd-unchanged";}
 assert(personal.SaveOrder(personal.Root(),order,error));
 PersonalFolders reopened;
 assert(reopened.ReadOrder(reopened.Root())==order);
 // Swapping visual positions does not alter filesystem structure.
 struct stat st{};
 assert(stat((a+"/obra.mid").c_str(),&st)==0);
 assert(stat((b+"/original.mid").c_str(),&st)==0);
 // 'Drop inside' is explicitly distinct and preserves destination songs.
 string moved;
 assert(personal.Move(a,b,moved,error));
 assert(moved==b+"/A");
 assert(stat((moved+"/obra.mid").c_str(),&st)==0);
 assert(stat((b+"/original.mid").c_str(),&st)==0);
 assert(stat(a.c_str(),&st)!=0);
 assert(!personal.Move(moved,moved,moved,error));
 assert(!personal.Move(b,moved,moved,error));
 std::cout<<"DROP_CHOICES_OK: exact swap, persisted order, nested folder, existing MIDI intact\n";
}
