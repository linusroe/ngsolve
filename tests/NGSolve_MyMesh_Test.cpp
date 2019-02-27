#include <mystdlib.h>
#include <mymeshinterface.hpp>
#include <mymesh.hpp>

#include <meshing.hpp>

#include <meshaccess.hpp>


class MyMesh;
class Ngx_MyMesh;

int main()
{
    MyMesh mym(2,3,2);
    Ngx_MyMesh ngx_mym(make_shared<MyMesh>(mym));
}