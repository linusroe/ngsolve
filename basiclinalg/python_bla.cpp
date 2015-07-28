#ifdef NGS_PYTHON
#include "../ngstd/python_ngstd.hpp"
#include <boost/python/slice.hpp>
#include <bla.hpp>
#include <numpy/arrayobject.h>

using namespace ngbla;

static void Init( const bp::slice &inds, int len, int &start, int &step, int &n ) {
    bp::object indices = inds.attr("indices")(len);
    start = 0;
    step = 1;
    n = 0;

    try {
        start = bp::extract<int>(indices[0]);
        int stop  = bp::extract<int>(indices[1]);
        step  = bp::extract<int>(indices[2]);
        n = (stop-start+step-1) / step;
    }
    catch (bp::error_already_set const &) {
        cout << "Error in Init(slice,...): " << endl;
        PyErr_Print();
    }

}


template <typename T, typename TNEW = T>
struct PyVecAccess : public boost::python::def_visitor<PyVecAccess<T, TNEW> > {
    typedef typename T::TSCAL TSCAL;

    template <class Tclass>
    void visit(Tclass& c) const {
        c.def("__getitem__", FunctionPointer( [](T &self, bp::slice inds )-> TNEW {
            int start, step, n;
            Init( inds, self.Size(), start, step, n );
            TNEW res(n);
            for (int i=0; i<n; i++, start+=step)
                res[i] = self[start];
            return res;
            } ) );
        c.def("__getitem__", FunctionPointer( [](T &v, bp::list ind )-> TNEW {
                int n = bp::len(ind);
                TNEW res(n);
                for (int i=0; i<n; i++) {
                    res[i] = v[ bp::extract<int>(ind[i]) ];
                }
                return res;
            } ) );
        c.def("__setitem__", FunctionPointer( [](T &self, bp::slice inds, const T & rv ) {
            int start, step, n;
            Init( inds, self.Size(), start, step, n );
            for (int i=0; i<n; i++, start+=step)
                self[start] = rv[i];
            } ) );
        c.def("__setitem__", FunctionPointer( [](T &self, bp::slice inds, TSCAL val ) {
            int start, step, n;
            Init( inds, self.Size(), start, step, n );
            for (int i=0; i<n; i++, start+=step)
                self[start] = val;
            } ) );
        c.def("__add__" , FunctionPointer( [](T &self, T &v) { return TNEW(self+v); }) );
        c.def("__sub__" , FunctionPointer( [](T &self, T &v) { return TNEW(self-v); }) );
        c.def("__mul__" , FunctionPointer( [](T &self, TSCAL s) { return TNEW(s*self); }) );
        c.def("__rmul__" , FunctionPointer( [](T &self, TSCAL s) { return TNEW(s*self); }) );
        c.def("InnerProduct", FunctionPointer ( [](T & x, T & y) { return InnerProduct (x, y); }));
    }
};



template <typename TMAT, typename TNEW=TMAT>
struct PyMatAccess : public boost::python::def_visitor<PyMatAccess<TMAT, TNEW> > {
    // TODO: correct typedefs
    typedef typename TMAT::TSCAL TSCAL;
    typedef Vector<TSCAL> TROW;
    typedef Vector<TSCAL> TCOL;

    template <class Tclass>
    void visit(Tclass& c) const {
        c.def("__getitem__", &PyMatAccess<TMAT, TNEW>::GetTuple);
        c.def("__getitem__", &PyMatAccess<TMAT, TNEW>::RowGetInt);
        c.def("__getitem__", &PyMatAccess<TMAT, TNEW>::RowGetSlice);

        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::SetTuple);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::SetTupleScal);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::SetTupleVec);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::RowSetInt);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::RowSetIntScal);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::RowSetSlice);
        c.def("__setitem__", &PyMatAccess<TMAT, TNEW>::RowSetSliceScal);

        c.add_property("diag",
                FunctionPointer( [](TMAT &self) { return Vector<TSCAL>(self.Diag()); }),
                FunctionPointer( [](TMAT &self, const FlatVector<TSCAL> &v) { self.Diag() = v; }));
        c.def("__add__" , FunctionPointer( [](TMAT &self, TMAT &m) { return TNEW(self+m); }) );
        c.def("__sub__" , FunctionPointer( [](TMAT &self, TMAT &m) { return TNEW(self-m); }) );
        c.def("__mul__" , FunctionPointer( [](TMAT &self, TMAT &m) { return TNEW(self*m); }) );
        c.def("__mul__" , FunctionPointer( [](TMAT &self, FlatVector<TSCAL> &v) { return Vector<TSCAL>(self*v); }) );
        c.def("__mul__" , FunctionPointer( [](TMAT &self, TSCAL s) { return TNEW(s*self); }) );
        c.def("__rmul__" , FunctionPointer( [](TMAT &self, TSCAL s) { return TNEW(s*self); }) );
        c.def("Height", &TMAT::Height );
        c.def("Width", &TMAT::Width );
        c.add_property("h", &TMAT::Height );
        c.add_property("w", &TMAT::Width );
        c.add_property("T", FunctionPointer( [](TMAT &self) { return TNEW(Trans(self)); }) ) ;
        c.add_property("A", FunctionPointer( [](TMAT &self) { return Vector<TSCAL>(FlatVector<TSCAL>( self.Width()* self.Height(), &self(0,0)) ); }) ) ;
        c.def("__len__", FunctionPointer( []( TMAT& self) { return self.Height();} ) );
    }

    static bp::object GetTuple( TMAT & self, bp::tuple t) {
        bp::object rows = t[0];
        bp::object cols = t[1];

        // First element of tuple is of type int
        bp::extract<int> row_ind(rows);
        if(row_ind.check()) {
            bp::object row( self.Row(row_ind()) );
            return row.attr("__getitem__")(cols);
        }

        // Second element of tuple is of type int
        bp::extract<int> col_ind(cols);
        if(col_ind.check()) {
            bp::object col( TROW(self.Col(col_ind())) );
            return col.attr("__getitem__")(rows);
        }

        // Both elements are slices
        try {
            bp::extract<bp::slice> row_slice(rows);
            bp::extract<bp::slice> col_slice(cols);
            return bp::object(ColGetSlice(RowGetSlice(self, row_slice()), col_slice()));
        } catch (bp::error_already_set const &) {
            cerr << "Invalid Matrix access!" << endl;
            PyErr_Print();
        }
        return bp::object();
    }

    static void SetTupleVec( TMAT & self, bp::tuple t, const FlatVector<TSCAL> &v) {
        bp::object rows = t[0];
        bp::object cols = t[1];

        // First element of tuple is of type int
        bp::extract<int> row_ind(rows);
        if(row_ind.check()) {
            bp::object row( self.Row(row_ind()) );
            row.attr("__setitem__")(cols, v);
            return;
        }

        // Second element of tuple is of type int
        bp::extract<int> col_ind(cols);
        if(col_ind.check()) {
            bp::extract<bp::slice> row_slice(rows);
            auto col = self.Col(col_ind());
            int start, step, n;
            Init( row_slice, self.Height(), start, step, n );
            for (int i=0; i<n; i++, start+=step)
                col[start] = v[i];
            return;
        }

        // One of the indices has to be of type int
        cerr << "Invalid Matrix access!" << endl;
    }

    static void SetTupleScal( TMAT & self, bp::tuple t, TSCAL val) {
        bp::object rows = t[0];
        bp::object cols = t[1];

        // First element of tuple is of type int
        bp::extract<int> row_ind(rows);
        if(row_ind.check()) {
            bp::object row( self.Row(row_ind()) );
            row.attr("__setitem__")(cols, val);
            return;
        }

        // Second element of tuple is of type int
        bp::extract<int> col_ind(cols);
        if(col_ind.check()) {
            bp::extract<bp::slice> row_slice(rows);
            auto col = self.Col(col_ind());
            int start, step, n;
            Init( row_slice, self.Height(), start, step, n );
            for (int i=0; i<n; i++, start+=step)
                col[start] = val;
            return;
        }

        // Both elements are slices
        try {
            bp::extract<bp::slice> row_slice(rows);
            int start, step, n;
            Init( row_slice(), self.Height(), start, step, n );
            for (int i=0; i<n; i++, start+=step) {
                bp::object row(self.Row(start));
                row.attr("__setitem__")(cols, val);
            }
        } catch (bp::error_already_set const &) {
            cerr << "Invalid Matrix access!" << endl;
            PyErr_Print();
        }
    }

    static void SetTuple( TMAT & self, bp::tuple t, const TMAT & rmat) {
        bp::object rows = t[0];
        bp::object cols = t[1];

        // Both elements have to be slices
        try {
            bp::extract<bp::slice> row_slice(rows);
            bp::extract<bp::slice> col_slice(cols);
            int start, step, n;
            Init( row_slice(), self.Height(), start, step, n );
            for (int i=0; i<n; i++, start+=step) {
                bp::object row(self.Row(start));
                row.attr("__setitem__")(cols, rmat.Row(i));
            }
        } catch (bp::error_already_set const &) {
            cerr << "Invalid Matrix access!" << endl;
            PyErr_Print();
        }
    }

    static TROW RowGetInt( TMAT & self,  int ind )  {
        return self.Row(ind);
    }

    static TNEW RowGetSlice( TMAT & self,  bp::slice inds ) {
        int start, step, n;
        Init( inds, self.Height(), start, step, n );
        TNEW res(n, self.Width());
        for (int i=0; i<n; i++, start+=step)
            res.Row(i) = self.Row(start);
        return res;
    }

    static void RowSetInt( TMAT & self,  int ind, const TROW &r ) {
        self.Row(ind) = r;
    }

    static void RowSetIntScal( TMAT & self,  int ind, TSCAL r ) {
        self.Row(ind) = r;
    }

    static void RowSetSlice( TMAT & self,  bp::slice inds, const TMAT &r ) {
        int start, step, n;
        Init( inds, self.Height(), start, step, n );
        for (int i=0; i<n; i++, start+=step)
            self.Row(start) = r.Row(i);
    }

    static void RowSetSliceScal( TMAT & self,  bp::slice inds, TSCAL r ) {
        int start, step, n;
        Init( inds, self.Height(), start, step, n );
        for (int i=0; i<n; i++, start+=step)
            self.Row(start) = r;
    }

    static Vector<TSCAL> ColGetInt( TMAT & self,  int ind )  {
        return Vector<TSCAL>(self.Col(ind));
    }

    static TNEW ColGetSlice( const TMAT & self,  bp::slice inds ) {
        int start, step, n;
        Init( inds, self.Width(), start, step, n );
        TNEW res(self.Height(),n);
        for (int i=0; i<n; i++, start+=step)
            res.Col(i) = self.Col(start);
        return res;
    }

    static void ColSetInt( TMAT & self,  int ind, const TCOL &r ) {
        self.Col(ind) = r;
    }

    static void ColSetIntScal( TMAT & self,  int ind, TSCAL r ) {
        self.Col(ind) = r;
    }

    static void ColSetSlice( TMAT & self,  bp::slice inds, const TMAT &r ) {
        int start, step, n;
        Init( inds, self.Width(), start, step, n );
        for (int i=0; i<n; i++, start+=step)
            self.Col(start) = r.Col(i);
    }

    static void ColSetSliceScal( TMAT & self,  bp::slice inds, TSCAL r ) {
        int start, step, n;
        Init( inds, self.Width(), start, step, n );
        for (int i=0; i<n; i++, start+=step)
            self.Col(start) = r;
    }
};

template <typename TVEC, typename TNEW, typename TSCAL>
auto ExportVector( const char * name ) -> bp::class_<TVEC>
  {
    return bp::class_<TVEC >(name, bp::no_init )
        .def(PyDefVector<TVEC, TSCAL>())
        .def(PyVecAccess< TVEC, TNEW >())
        .def(PyDefToString<TVEC >())
        .def(bp::self+=bp::self)
        .def(bp::self-=bp::self)
        .def(bp::self*=TSCAL())
        ;
  }

void NGS_DLL_HEADER ExportNgbla() {
    std::string nested_name = "bla";
    if( bp::scope() )
         nested_name = bp::extract<std::string>(bp::scope().attr("__name__") + ".bla");

    bp::object module(bp::handle<>(bp::borrowed(PyImport_AddModule(nested_name.c_str()))));

    cout << "exporting bla as " << nested_name << endl;
    bp::object parent = bp::scope() ? bp::scope() : bp::import("__main__");
    parent.attr("bla") = module ;

    bp::scope ngbla_scope(module);
    _import_array();
    ///////////////////////////////////////////////////////////////////////////////////////
    // Vector types
    typedef FlatVector<double> FVD;
    typedef FlatVector<Complex> FVC;
    typedef SliceVector<double> SVD;
    typedef SliceVector<Complex> SVC;
    typedef Vector<double> VD;
    typedef Vector<Complex> VC;

    ExportVector< FVD, VD, double>("FlatVectorD")
        .def(bp::init<int, double *>())
        .def("Range",    static_cast</* const */ FVD (FVD::*)(int,int) const> (&FVD::Range ) )
        .def("NumPy", FunctionPointer([] (FVD & self)
                                   {
                                     npy_intp dims[] = { self.Size() };
                                     PyObject * ob = PyArray_SimpleNewFromData (1, dims, NPY_DOUBLE, &self(0));
                                     return ob;
                                   }))
        ;
    ExportVector< FVC, VC, Complex>("FlatVectorC")
        .def(bp::self*=double())
        .def(bp::init<int, Complex *>())
        .def("Range",    static_cast</* const */ FVC (FVC::*)(int,int) const> (&FVC::Range ) )
        ;

    ExportVector< SVD, VD, double>("SliceVectorD")
        .def("Range",    static_cast<const SVD (SVD::*)(int,int) const> (&SVD::Range ) )
        ;
    ExportVector< SVC, VC, Complex>("SliceVectorC")
        .def("Range",    static_cast<const SVC (SVC::*)(int,int) const> (&SVC::Range ) )
        .def(bp::self*=double())
        ;

    bp::class_<Vector<double>,  bp::bases<FlatVector<double> > >("VectorD")
        .def(bp::init<int>())
        ;

    bp::class_<Vector<Complex>,  bp::bases<FlatVector<Complex> > >("VectorC")
        .def(bp::init<int>())
        ;

    bp::def("Vector",
            FunctionPointer( [] (int n, bool is_complex) {
                if(is_complex) return bp::object(Vector<Complex>(n));
                else return bp::object(Vector<double>(n));
                }),
            (boost::python::arg("length"),
            boost::python::arg("complex")=false)
           );


    ///////////////////////////////////////////////////////////////////////////////////////
    // Matrix types
    typedef FlatMatrix<double> FMD;
    bp::class_<FlatMatrix<double> >("FlatMatrixD")
        .def(PyDefToString<FMD>())
        .def(PyMatAccess<FMD, Matrix<double> >())
        .def(bp::self+=bp::self)
        .def(bp::self-=bp::self)
        .def(bp::self*=double())
        .def("NumPy", FunctionPointer([] (FMD & self)
                                   {
                                     npy_intp dims[] = { self.Height(), self.Width() };
                                     PyObject * ob = PyArray_SimpleNewFromData (2, dims, NPY_DOUBLE, &self(0,0));
                                     return ob;
                                   }))

        ;


    typedef FlatMatrix<Complex> FMC;
    bp::class_<FlatMatrix<Complex> >("FlatMatrixC")
        .def(PyDefToString<FMC>())
        .def(PyMatAccess<FMC, Matrix<Complex> >())
        .def(bp::self+=bp::self)
        .def(bp::self-=bp::self)
        .def(bp::self*=Complex())
        .add_property("diag",
                FunctionPointer( [](FMC &self) { return Vector<Complex>(self.Diag()); }),
                FunctionPointer( [](FMC &self, const FVC &v) { self.Diag() = v; }))
        .def("__add__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(self+m); }) )
        .def("__sub__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(self-m); }) )
        .def("__mul__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(self*m); }) )
        .def("__radd__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(self+m); }) )
        .def("__rsub__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(self-m); }) )
        .def("__rmul__" , FunctionPointer( [](FMC &self, FMD &m) { return Matrix<Complex>(m*self); }) )
        .def("__mul__" , FunctionPointer( [](FMC &self, FVD &v) { return Vector<Complex>(self*v); }) )
        .def("__mul__" , FunctionPointer( [](FMC &self, double s) { return Matrix<Complex>(s*self); }) )
        .def("__rmul__" , FunctionPointer( [](FMC &self, double s) { return Matrix<Complex>(s*self); }) )
        .def("Height", &FMC::Height )
        .def("Width", &FMC::Width )
        .def("__len__", FunctionPointer( []( FMC& self) { return self.Height();} ) )
        .add_property("h", &FMC::Height )
        .add_property("w", &FMC::Width )
        .add_property("A", FunctionPointer( [](FMC &self) { return Vector<Complex>(FlatVector<Complex>( self.Width()* self.Height(), &self(0,0) )); })  )
        .add_property("T", FunctionPointer( [](FMC &self) { return Matrix<Complex>(Trans(self)); }) ) 
        .add_property("C", FunctionPointer( [](FMC &self) { 
            Matrix<Complex> result( self.Height(), self.Width() );
            for (int i=0; i<self.Height(); i++)
                for (int j=0; j<self.Width(); j++) 
                    result(i,j) = Conj(self(i,j));
            return result;
            }) ) 
        .add_property("H", FunctionPointer( [](FMC &self) { 
            Matrix<Complex> result( self.Width(), self.Height() );
            for (int i=0; i<self.Height(); i++)
                for (int j=0; j<self.Width(); j++) 
                    result(j,i) = Conj(self(i,j));
            return result;
            }) ) 
        ;

    bp::class_<Matrix<double>, bp::bases<FMD> >("MatrixD")
        .def(bp::init<int, int>())
        ;

    bp::class_<Matrix<Complex>, bp::bases<FMC> >("MatrixC")
        .def(bp::init<int, int>())
        ;

    bp::def("Matrix",
            FunctionPointer( [] (int h, int w, bool is_complex) {
                if(is_complex) return bp::object(Matrix<Complex>(h,w));
                else return bp::object(Matrix<double>(h,w));
                }),
            (boost::python::arg("height"), 
            boost::python::arg("width"), 
            boost::python::arg("complex")=false)
           );

    bp::def ("InnerProduct",
             FunctionPointer( [] (bp::object x, bp::object y) -> bp::object
                              { return x.attr("InnerProduct") (y); }));
}


BOOST_PYTHON_MODULE(libngbla) {
    ExportNgbla();
}

#endif // NGS_PYTHON
