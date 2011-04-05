#include "MakeModel.h"
#include<fstream>


int main(int argc, char ** argv){

std::ifstream fin("makemodel.model");
std::ofstream fout("log.txt");
MakeModel* makeModel = new MakeModel("mymodel", fin, fout);

std::ofstream output("output.txt");
makeModel->print_contents(output);
std::cout<<makeModel->getExp("arch/alpha/kernel/irq_i8259.c")<<std::endl;
return 0;
}
