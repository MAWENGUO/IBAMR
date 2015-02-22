// Filename: QuadratureAnisotropicCompositeGauss.cpp
// Created on 22 Feb 2015 by Wenjun Kou
//
// Copyright (c) 2002-2015, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include "ibtk/QuadratureAnisotropicCompositeGauss.h"


namespace libMesh
{

void QuadratureAnisotropicCompositeGauss::buildStandard1D( std::vector<Real> & standard_weights,  std::vector<Real> & standard_cods)
{
 
   switch(d_num_qps) // number of means points
  {
  
    case 1:
      {
	
	standard_cods.resize (1);
	standard_weights.resize(1);

	standard_cods[0]  = 0.;

	standard_weights[0]    = 2.;

	return;
      }
    case 2:
    //case 3:
      {
	standard_cods.resize (2);
	standard_weights.resize(2);

	standard_cods[0] = -5.7735026918962576450914878050196e-01L; // -sqrt(3)/3
	standard_cods[1]    = -standard_cods[0];

	standard_weights[0]   = 1.;
	standard_weights[1]   = standard_weights[0];

	return;
      }
    case 3:
    //case 5:
      {
	standard_cods.resize (3);
	standard_weights.resize(3);

	standard_cods[ 0]= -7.7459666924148337703585307995648e-01L;
	standard_cods[ 1]= 0.;
	standard_cods[ 2]    = -standard_cods[0];

	standard_weights[ 0]   = 5.5555555555555555555555555555556e-01L;
	standard_weights[ 1]   = 8.8888888888888888888888888888889e-01L;
	standard_weights[ 2]   = standard_weights[0];

	return;
      }
    case 4:
    //case 7:
      {
	standard_cods.resize (4);
	standard_weights.resize(4);

	standard_cods[ 0]= -8.6113631159405257522394648889281e-01L;
	standard_cods[ 1]= -3.3998104358485626480266575910324e-01L;
	standard_cods[ 2]    = -standard_cods[1];
	standard_cods[ 3]    = -standard_cods[0];

	standard_weights[ 0]   = 3.4785484513745385737306394922200e-01L;
	standard_weights[ 1]   = 6.5214515486254614262693605077800e-01L;
	standard_weights[ 2]   = standard_weights[1];
	standard_weights[ 3]   = standard_weights[0];

	return;
      }
    case 5:
    //case 9:
      {
	standard_cods.resize (5);
	standard_weights.resize(5);

	standard_cods[ 0]= -9.0617984593866399279762687829939e-01L;
	standard_cods[ 1]= -5.3846931010568309103631442070021e-01L;
	standard_cods[ 2]= 0.;
	standard_cods[ 3]    = -standard_cods[1];
	standard_cods[ 4]    = -standard_cods[0];

	standard_weights[ 0]   = 2.3692688505618908751426404071992e-01L;
	standard_weights[ 1]   = 4.7862867049936646804129151483564e-01L;
	standard_weights[ 2]   = 5.6888888888888888888888888888889e-01L;
	standard_weights[ 3]   = standard_weights[1];
	standard_weights[ 4]   = standard_weights[0];

	return;
      }
    default:
      {
	libMesh::err << "ERROR:only support n_pts in (1,5), not this one: " << d_num_qps << std::endl;
	libmesh_error();
      
      }
      
  } // switch

  libmesh_error();  
  return;
}


void QuadratureAnisotropicCompositeGauss::init_1D(const ElemType,
                    unsigned int)
{
  //----------------------------------------------------------------------
  // 1D quadrature rules
  // here _order means number of points
 if ((!d_use_composite) || (!d_use_anisotropic) || (d_num_qps <1))
 {	
   libMesh::err << "ERROR:input is invalid for one anisotropic composite qp rule " << std::endl;
   libmesh_error();
   return ;
 }
 
 // step 1: build_standard_1D
 std::vector<Real> standard_weights;
 std::vector<Real> standard_cods;
 
 buildStandard1D( standard_weights,  standard_cods);

  unsigned int cur_num_intervals = _order;
  unsigned int total_qps = cur_num_intervals * d_num_qps;
  double h = 2.0 / cur_num_intervals; // sub-interval size

  _weights.resize(total_qps);
    
  _points.resize(total_qps);

  unsigned int id_interval;
  for (id_interval =0; id_interval < cur_num_intervals; id_interval++)
  {
    unsigned int start_index = id_interval * d_num_qps;
    double x1 = -1.0 + h *id_interval;
    double x2 = x1 + h;
    
    tranform1D( _weights, _points, start_index, 
		   standard_weights, standard_cods, x1, x2);
  }
 
  return; 
 
}


void QuadratureAnisotropicCompositeGauss::init_2D(const ElemType type_in,
                    unsigned int)
{
#if LIBMESH_DIM > 1

  //-----------------------------------------------------------------------
  // 2D quadrature rules

    
  if ((!d_use_composite) || (!d_use_anisotropic) || (d_num_qps <1))//(!d_use_anisotropic)
    {
  

      //---------------------------------------------
      // Unsupported type
   
	libMesh::err << "Only support anisotropic composite qp rule!:" << type_in << std::endl;
	libmesh_error();
	return;
    }

//libmesh_error();
  switch (type_in)
    {


      //---------------------------------------------
      // Quadrilateral quadrature rules
    case QUAD4:
    case QUAD8:
    case QUAD9:
      {
	// We compute the 2D quadrature rule as a tensor
	// product of the 1D quadrature rule.

	QuadratureAnisotropicCompositeGauss q1D1(1,d_vec_order[0], d_num_qps); // number of intervals
	q1D1.init(EDGE2);
	
	QuadratureAnisotropicCompositeGauss q1D2(1,d_vec_order[1], d_num_qps); //d_vec_order[1]);
	q1D2.init(EDGE2);	

	tensorProductForQuad( q1D1, q1D2 );
	

	return;
      }
      //---------------------------------------------
      // Unsupported type
    default:
      {
	libMesh::err << "Only suppoer Quad_element: Element type not supported!:" << type_in << std::endl;
	libmesh_error();
      }
    }  
 libmesh_error();     
 return;

#endif
}



void QuadratureAnisotropicCompositeGauss::init_3D(const ElemType type_in,
                    unsigned int)
{
#if LIBMESH_DIM == 3

  //-----------------------------------------------------------------------
  // 3D quadrature rules
    if ((!d_use_composite) || (!d_use_anisotropic) || (d_num_qps <1))//(!QuadratureAnisotropicCompositeGauss::d_use_anisotropic)
    {
  

      //---------------------------------------------
      // Unsupported type
   
	libMesh::err << "Only support anisotropic composite qp rule!:" << type_in << std::endl;
	libmesh_error();
	return;
    }
    
  switch (type_in)
    {
      //---------------------------------------------
      // Hex quadrature rules
    case HEX8:
    case HEX20:
    case HEX27:
      {
	// We compute the 3D quadrature rule as a tensor
	// product of the 1D quadrature rule.

	QuadratureAnisotropicCompositeGauss q1D1(1,d_vec_order[0], d_num_qps); 
	q1D1.init(EDGE2);
	
	QuadratureAnisotropicCompositeGauss q1D2(1,d_vec_order[1], d_num_qps); 
	q1D2.init(EDGE2);
	
	QuadratureAnisotropicCompositeGauss q1D3(1,d_vec_order[2], d_num_qps); 
	q1D3.init(EDGE2);
	
	tensorProductForHex( q1D1,q1D2,q1D3 );

	return;
      }



      

      //---------------------------------------------
      // Unsupported type
    default:
      {
	libMesh::err << "ERROR:Only suppoer Hex_element: Unsupported type: " << type_in << std::endl;
	libmesh_error();
      }
    }

  libmesh_error();

  return;

#endif
}


} // namespace libMesh
