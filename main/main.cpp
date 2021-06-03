#include "common.hpp"
#include "builder.hpp"

int main(int, char**)
{
    fontatlas::ScopeCoInitialize co;
    
    fontatlas::Builder builder;
    
    builder.addFont("Sans24", "SourceHanSansSC-Regular.otf", 0, 24);
    builder.addRange("Sans24", 32, 126);
    builder.addRange("Sans24", 0x4E00, 0x9FFF);
    
    //builder.addFont("System24", "C:\\Windows\\Fonts\\msyh.ttc", 0, 24);
    //builder.addText("System24", "System.IO.FileStream: 无法打开文件");
    
    builder.build("font\\", false, 2048, 2048, 1, 0);
    
    return 0;
}
