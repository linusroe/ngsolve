#ifdef NGS_PYTHON
#include "../ngstd/python_ngstd.hpp"
#include <comp.hpp>
using namespace ngcomp;



template <> class cl_NonElement<ElementId>
{
public:
  static ElementId Val() { return ElementId(VOL,-1); }
};



BOOST_PYTHON_MODULE(Ngcomp) {

  cout << "init py - ngcomp" << endl;


  bp::enum_<VorB>("VorB")
    .value("VOL", VOL)
    .value("BND", BND)
    .export_values()
    ;

  bp::class_<ElementId> ("ElementId", bp::init<VorB,int>())
    .def(PyDefToString<ElementId>())
    .add_property("nr", &ElementId::Nr)    
    .def("IsVolume", &ElementId::IsVolume)
    .def("IsBoundary", &ElementId::IsBoundary)
    .def(PyDefToString<FESpace>())
    ;
  //
  bp::class_<ElementRange,bp::bases<IntRange>> ("ElementRange",bp::init<VorB,IntRange>())
    .def(PyDefIterable2<ElementRange>())
    // .def("__iter__", bp::iterator<ElementRange>())
    ;

  bp::class_<Ngs_Element>("Ngs_Element", bp::no_init)
    .add_property("vertices", FunctionPointer([](Ngs_Element &el)->Array<int>{return Array<int>(el.Vertices());} ))
    .add_property("edges", FunctionPointer([](Ngs_Element &el)->Array<int>{ return Array<int>(el.Edges());} ))
    .add_property("faces", FunctionPointer([](Ngs_Element &el)->Array<int>{ return Array<int>(el.Faces());} ))
    .add_property("type", &Ngs_Element::GetType)
    ;

  bp::class_<MeshAccess>("Mesh", "meshclass doc", bp::init<string>())
    .def("LoadMesh", static_cast<void(MeshAccess::*)(const string &)>(&MeshAccess::LoadMesh))
    .def("Elements", static_cast<ElementRange(MeshAccess::*)(VorB)const> (&MeshAccess::Elements))
    .def("GetElement", static_cast<Ngs_Element(MeshAccess::*)(ElementId)const> (&MeshAccess::GetElement))
    .def("__getitem__", static_cast<Ngs_Element(MeshAccess::*)(ElementId)const> (&MeshAccess::operator[]))

    .def ("GetNE", static_cast<int(MeshAccess::*)(VorB)const> (&MeshAccess::GetNE))
    .add_property ("nv", &MeshAccess::GetNV, "number of vertices")
    .add_property ("ne", static_cast<int(MeshAccess::*)()const> (&MeshAccess::GetNE))

    .def ("GetTrafo", 
          static_cast<ElementTransformation&(MeshAccess::*)(ElementId,LocalHeap&)const>
          (&MeshAccess::GetTrafo), 
          bp::return_value_policy<bp::reference_existing_object>())

    // first attempts, but keep for a moment ...
    .def("GetElement", static_cast< Ngs_Element (MeshAccess::*)(int, bool)const> (&MeshAccess::GetElement),
         (bp::arg("arg1")=NULL,bp::arg("arg2")=0))
    .def("GetElementVertices", static_cast<void (MeshAccess::*)(int, Array<int> &) const>( &MeshAccess::GetElVertices))


    ;


  bp::class_<FESpace, shared_ptr<FESpace>,  boost::noncopyable>("FESpace", bp::no_init)
    .def("Update", FunctionPointer([](FESpace & self, int heapsize)
                                   {
                                     LocalHeap lh (heapsize, "FESpace::Update-heap");
                                     self.Update(lh);
                                     self.FinalizeUpdate(lh);
                                   }),
         (bp::arg("self")=NULL,bp::arg("heapsize")=1000000))
    
    .def("GetDofNrs", FunctionPointer([](FESpace & self, int i) { Array<int> tmp; self.GetDofNrs(i,tmp); return tmp; }))
    .def("GetDofNrs", FunctionPointer([](FESpace & self, ElementId i) { Array<int> tmp; self.GetDofNrs(i,tmp); return tmp; }))
    .add_property ("ndof", FunctionPointer([](FESpace & self) { return self.GetNDof(); }))
    .def(PyDefToString<FESpace>())
    .def ("GetFE", 
          static_cast<const FiniteElement&(FESpace::*)(ElementId,LocalHeap&)const>
          (&FESpace::GetFE), 
          bp::return_value_policy<bp::reference_existing_object>())
    ;
  
  bp::class_<GridFunction, shared_ptr<GridFunction>, boost::noncopyable>("GridFunction", bp::no_init)
    .def(PyDefToString<GridFunction>())
   .def("Update", FunctionPointer([](GridFunction & self)
                                   {
                                     self.Update();
                                   }))
    .def("Vector", FunctionPointer([](shared_ptr<GridFunction> self)->BaseVector& { return self->GetVector(); }),
         bp::return_value_policy<bp::reference_existing_object>())


    // .def("Vector", &GridFunction::GetVector, 
    //     bp::return_value_policy<bp::reference_existing_object>())
    ;

  bp::def("CreateGF", FunctionPointer
          ([](string name, const FESpace & fespace)
           {
             Flags flags;
             GridFunction * gf = CreateGridFunction (&fespace, name, flags);
             return shared_ptr<GridFunction> (gf);
           }),
          (bp::arg("name"), bp::arg("fespace")));
  
  bp::class_<BilinearForm, shared_ptr<BilinearForm>, boost::noncopyable>("BilinearForm", bp::no_init)
    .def(PyDefToString<BilinearForm>())
    .def("Matrix", FunctionPointer([](shared_ptr<BilinearForm> self)->BaseMatrix& { return self->GetMatrix(); }),
         bp::return_value_policy<bp::reference_existing_object>())
    ;

  bp::class_<LinearForm, shared_ptr<LinearForm>, boost::noncopyable>("LinearForm", bp::no_init)
    .def(PyDefToString<LinearForm>())
    .def("Vector", FunctionPointer([](shared_ptr<LinearForm> self)->BaseVector& { return self->GetVector(); }),
         bp::return_value_policy<bp::reference_existing_object>())
    ;


}






BOOST_PYTHON_MODULE(libngcomp)
{
  cout << "execute 'import Ngstd' to import py-ngstd module" << endl;
  cout << "execute 'import Ngbla' to import py-ngbla module" << endl;
  cout << "execute 'import Ngfem' to import py-ngfem module" << endl;
  cout << "execute 'import Ngcomp' to import py-ngcomp module" << endl;
}

struct Init {
  Init() 
  { 
    PyImport_AppendInittab("Ngcomp", PyInit_Ngcomp); 
  }
};
static Init init;







#endif // NGS_PYTHON
