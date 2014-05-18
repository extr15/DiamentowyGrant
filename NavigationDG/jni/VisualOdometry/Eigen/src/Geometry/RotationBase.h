// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#ifndef EIGEN_ROTATIONBASE_H
#define EIGEN_ROTATIONBASE_H

// forward declaration
template<typename RotationDerived, typename MatrixType, bool IsVector=MatrixType::IsVectorAtCompileTime>
struct ei_rotation_base_generic_product_selector;

/** \class RotationBase
  *
  * \brief Common base class for compact rotation representations
  *
  * \param Derived is the derived type, i.e., a rotation type
  * \param _Dim the dimension of the space
  */
template<typename Derived, int _Dim>
class RotationBase
{
  public:
    enum { Dim = _Dim };
    /** the scalar type of the coefficients */
    typedef typename ei_traits<Derived>::Scalar Scalar;

    /** corresponding linear transformation matrix type */
    typedef Matrix<Scalar,Dim,Dim> RotationMatrixType;
    typedef Matrix<Scalar,Dim,1> VectorType;

  public:
    inline const Derived& derived() const { return *static_cast<const Derived*>(this); }
    inline Derived& derived() { return *static_cast<Derived*>(this); }

    /** \returns an equivalent rotation matrix */
    inline RotationMatrixType toRotationMatrix() const { return derived().toRotationMatrix(); }

    /** \returns an equivalent rotation matrix 
      * This function is added to be conform with the Transform class' naming scheme.
      */
    inline RotationMatrixType matrix() const { return derived().toRotationMatrix(); }

    /** \returns the inverse rotation */
    inline Derived inverse() const { return derived().inverse(); }

    /** \returns the concatenation of the rotation \c *this with a translation \a t */
    inline Transform<Scalar,Dim,Isometry> operator*(const Translation<Scalar,Dim>& t) const
    { return Transform<Scalar,Dim,Isometry>(*this) * t; }

    /** \returns the concatenation of the rotation \c *this with a uniform scaling \a s */
    inline RotationMatrixType operator*(const UniformScaling<Scalar>& s) const
    { return toRotationMatrix() * s.factor(); }

    /** \returns the concatenation of the rotation \c *this with a generic expression \a e
      * \a e can be:
      *  - a DimxDim linear transformation matrix
      *  - a DimxDim diagonal matrix (axis aligned scaling)
      *  - a vector of size Dim
      */
    template<typename OtherDerived>
    EIGEN_STRONG_INLINE typename ei_rotation_base_generic_product_selector<Derived,OtherDerived,OtherDerived::IsVectorAtCompileTime>::ReturnType
    operator*(const EigenBase<OtherDerived>& e) const
    { return ei_rotation_base_generic_product_selector<Derived,OtherDerived>::run(derived(), e.derived()); }

    /** \returns the concatenation of a linear transformation \a l with the rotation \a r */
    template<typename OtherDerived> friend
    inline RotationMatrixType operator*(const EigenBase<OtherDerived>& l, const Derived& r)
    { return l.derived() * r.toRotationMatrix(); }

    /** \returns the concatenation of a scaling \a l with the rotation \a r */
    friend inline Transform<Scalar,Dim,Affine> operator*(const DiagonalMatrix<Scalar,Dim>& l, const Derived& r)
    { 
      Transform<Scalar,Dim,Affine> res(r);
      res.linear().applyOnTheLeft(l);
      return res;
    }

    /** \returns the concatenation of the rotation \c *this with a transformation \a t */
    template<int Mode>
    inline Transform<Scalar,Dim,Mode> operator*(const Transform<Scalar,Dim,Mode>& t) const
    { return toRotationMatrix() * t; }

    template<typename OtherVectorType>
    inline VectorType _transformVector(const OtherVectorType& v) const
    { return toRotationMatrix() * v; }
};

// implementation of the generic product rotation * matrix
template<typename RotationDerived, typename MatrixType>
struct ei_rotation_base_generic_product_selector<RotationDerived,MatrixType,false>
{
  enum { Dim = RotationDerived::Dim };
  typedef Matrix<typename RotationDerived::Scalar,Dim,Dim> ReturnType;
  inline static ReturnType run(const RotationDerived& r, const MatrixType& m)
  { return r.toRotationMatrix() * m; }
};

template<typename RotationDerived, typename Scalar, int Dim, int MaxDim>
struct ei_rotation_base_generic_product_selector< RotationDerived, DiagonalMatrix<Scalar,Dim,MaxDim>, false >
{
  typedef Transform<Scalar,Dim,Affine> ReturnType;
  inline static ReturnType run(const RotationDerived& r, const DiagonalMatrix<Scalar,Dim,MaxDim>& m)
  {
    ReturnType res(r);
    res.linear() *= m;
    return res;
  }
};

template<typename RotationDerived,typename OtherVectorType>
struct ei_rotation_base_generic_product_selector<RotationDerived,OtherVectorType,true>
{
  enum { Dim = RotationDerived::Dim };
  typedef Matrix<typename RotationDerived::Scalar,Dim,1> ReturnType;
  EIGEN_STRONG_INLINE static ReturnType run(const RotationDerived& r, const OtherVectorType& v)
  {
    return r._transformVector(v);
  }
};

/** \geometry_module
  *
  * \brief Constructs a Dim x Dim rotation matrix from the rotation \a r
  */
template<typename _Scalar, int _Rows, int _Cols, int _Storage, int _MaxRows, int _MaxCols>
template<typename OtherDerived>
Matrix<_Scalar, _Rows, _Cols, _Storage, _MaxRows, _MaxCols>
::Matrix(const RotationBase<OtherDerived,ColsAtCompileTime>& r)
{
  EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix,int(OtherDerived::Dim),int(OtherDerived::Dim))
  *this = r.toRotationMatrix();
}

/** \geometry_module
  *
  * \brief Set a Dim x Dim rotation matrix from the rotation \a r
  */
template<typename _Scalar, int _Rows, int _Cols, int _Storage, int _MaxRows, int _MaxCols>
template<typename OtherDerived>
Matrix<_Scalar, _Rows, _Cols, _Storage, _MaxRows, _MaxCols>&
Matrix<_Scalar, _Rows, _Cols, _Storage, _MaxRows, _MaxCols>
::operator=(const RotationBase<OtherDerived,ColsAtCompileTime>& r)
{
  EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix,int(OtherDerived::Dim),int(OtherDerived::Dim))
  return *this = r.toRotationMatrix();
}

/** \internal
  *
  * Helper function to return an arbitrary rotation object to a rotation matrix.
  *
  * \param Scalar the numeric type of the matrix coefficients
  * \param Dim the dimension of the current space
  *
  * It returns a Dim x Dim fixed size matrix.
  *
  * Default specializations are provided for:
  *   - any scalar type (2D),
  *   - any matrix expression,
  *   - any type based on RotationBase (e.g., Quaternion, AngleAxis, Rotation2D)
  *
  * Currently ei_toRotationMatrix is only used by Transform.
  *
  * \sa class Transform, class Rotation2D, class Quaternion, class AngleAxis
  */
template<typename Scalar, int Dim>
inline static Matrix<Scalar,2,2> ei_toRotationMatrix(const Scalar& s)
{
  EIGEN_STATIC_ASSERT(Dim==2,YOU_MADE_A_PROGRAMMING_MISTAKE)
  return Rotation2D<Scalar>(s).toRotationMatrix();
}

template<typename Scalar, int Dim, typename OtherDerived>
inline static Matrix<Scalar,Dim,Dim> ei_toRotationMatrix(const RotationBase<OtherDerived,Dim>& r)
{
  return r.toRotationMatrix();
}

template<typename Scalar, int Dim, typename OtherDerived>
inline static const MatrixBase<OtherDerived>& ei_toRotationMatrix(const MatrixBase<OtherDerived>& mat)
{
  EIGEN_STATIC_ASSERT(OtherDerived::RowsAtCompileTime==Dim && OtherDerived::ColsAtCompileTime==Dim,
    YOU_MADE_A_PROGRAMMING_MISTAKE)
  return mat;
}

#endif // EIGEN_ROTATIONBASE_H
