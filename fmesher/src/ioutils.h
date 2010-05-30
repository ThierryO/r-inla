#ifndef _FMESH_IOUTILS_
#define _FMESH_IOUTILS_ 1

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <string>

#include "vector.h"

#ifndef NOT_IMPLEMENTED
#define NOT_IMPLEMENTED (std::cout					\
			 << __FILE__ << "(" << __LINE__ << ")\t"	\
			 << "NOT IMPLEMENTED: "				\
			 << __PRETTY_FUNCTION__ << std::endl);
#endif

#define IOHEADER_VERSION 0
#define ASCII_DEFAULT true

namespace fmesh {

  /*! dense/sparse/map */
  enum IODatatype {IODatatype_dense=0,
		   IODatatype_sparse=1};
  /*! int/double */
  enum IOValuetype {IOValuetype_int=0,
		    IOValuetype_double=1};
  /*! general/symmentric/diagonal */
  enum IOMatrixtype {IOMatrixtype_general=0,
		     IOMatrixtype_symmetric=1,
		     IOMatrixtype_diagonal=2};
  /*! rowmajor/colmajor */
  enum IOStoragetype {IOStoragetype_rowmajor=0,
		      IOStoragetype_colmajor=1};

  /*! Header for input and output file formats. */
  class IOHeader {
  public:
    int version; /*!< Format version */
    int elems; /*!< The number of data units
		 
		 For dense matrices, the total number of elements.
		 For sparse matrices, the number of elements contained in
		 the file.
		*/
    int rows; /*!< The number of data rows. */
    int cols; /*!< The number of data columns. */
    int datatype; /*!< The IODatatype. */
    int valuetype; /*!< The IOValuetype. */
    int matrixtype; /*!< The IOMatrixtype. */
    int storagetype; /*!< The IOStoragetype. */

    /* Default values: */
    template <class T>
    IOHeader& DefaultDense(const Matrix<T>& M,
			   IOMatrixtype matrixt = IOMatrixtype_general);
    template <class T>
    IOHeader& DefaultSparse(const SparseMatrix<T>& M,
			    IOMatrixtype matrixt = IOMatrixtype_general);
    
    /* Constructor, that sets the valuetype matching T: */
    template <class T>
    IOHeader(const T& ref);
  };

  template <>
  IOHeader::IOHeader(const int& ref);
  template <>
  IOHeader::IOHeader(const double& ref);

  /*! Helper for input and output. */
  class IOHelper {
  public:
    IOHeader h_;
    bool ascii_;
  public:
    /* Constructors: */
    IOHelper() : h_(0), ascii_(ASCII_DEFAULT) {};
    IOHelper(const IOHeader& h) : h_(h), ascii_(ASCII_DEFAULT) {};
    template <class T>
    IOHelper(const Matrix<T>& M,
	     IOMatrixtype matrixt = IOMatrixtype_general,
	     bool set_ascii = ASCII_DEFAULT)
      : h_(T()), ascii_(set_ascii)
    { h_.DefaultDense(M,matrixt); };
    template <class T>
    IOHelper(const SparseMatrix<T>& M,
	     IOMatrixtype matrixt = IOMatrixtype_general,
	     bool set_ascii = ASCII_DEFAULT)
      : h_(T()), ascii_(set_ascii)
    { h_.DefaultSparse(M,matrixt); };

    IOHelper& ascii() {
      ascii_ = !ascii_;
      return *this;
    };
    IOHelper& storage() {
      if (h_.storagetype == IOStoragetype_rowmajor)
	h_.storagetype = IOStoragetype_colmajor;
      else
	h_.storagetype = IOStoragetype_rowmajor;
      return *this;
    };
    IOHelper& ascii(bool set_ascii) {
      ascii_ = set_ascii;
      return *this;
    };
    IOHelper& storage(bool set_storage) {
      h_.storagetype = set_storage;
      return *this;
    };

    /* Output/Input: */
    IOHelper& O(std::ostream& output);
    IOHelper& I(std::istream& input);
    IOHelper& H(const IOHeader& h);
    template <class T>
    IOHelper& O(std::ostream& output,
		const Matrix<T>& M);
    template <class T>
    IOHelper& I(std::istream& input,
		Matrix<T>& M);
    template <class T>
    IOHelper& O(std::ostream& output,
		const SparseMatrix<T>& M);
    template <class T>
    IOHelper& I(std::istream& input,
		SparseMatrix<T>& M);

  };



} /* namespace fmesh */

#include "ioutils.tpp"

#endif
