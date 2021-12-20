#include <Types.hpp>

int main(int argc, char** argv){

    std::ofstream file("teste.json",std::ios_base::openmode::_S_out);

    if(!file.good()) return -1;

    JsonDocument document;

    std::cout<<"[saved]\n";
    std::cout<<document<<std::endl;
    file<<document<<std::endl;

    file.close();

    return 0;
}
