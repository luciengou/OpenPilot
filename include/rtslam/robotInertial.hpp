/**
 * \file robotInertial.hpp
 *
 * \date 26/03/2010
 * \author jsola@laas.fr
 *
 *
 *  This file describes the class RobotInertial for a robot driven by inertial measurements.
 *
 * \ingroup rtslam
 */

#ifndef ROBOTINERTIAL_HPP_
#define ROBOTINERTIAL_HPP_

#include "jmath/jblas.hpp"
#include "boost/shared_ptr.hpp"
#include "rtslam/robotAbstract.hpp"

namespace jafar {
	namespace rtslam {
		using namespace std;

		class RobotInertial;
		typedef boost::shared_ptr<RobotInertial> inertial_ptr_t;


		/**
		 * Inertial measurements unit - robot motion model.
		 * \author jsola@laas.fr
		 *
		 * This motion model is driven by IMU measurements and random perturbations, and defined by:
		 * - The state vector: position, velocity, orientation quaternion, accelerometer bias, gyrometer bias, gravity:
		 * 		- x = [p q v ab wb g] , of size 19.
		 *
		 * - The transition equation
		 * 		- x+ = move_func(x,u)  -- which is implemented with internal variables, see move_func().
		 *
		 * with \a u=control.x() the control vector
		 * 		- u = [am, wm, ar, wr]
		 *
		 * stacking:
		 * - \a am: the acceleration measurements, with noise
		 * - \a wm: the gyrometer measurements, with noise
		 * - \a ar: the accelerometer bias random walk noise
		 * - \a wr: the gyrometer bias random walk noise
		 *
		 * The motion model equation x+ = f(x,u) is decomposed as:
		 * - p+  = p + v*dt
		 * - v+  = v + R(q)*(am - ab) + g     <-- am and wm: IMU measurements
		 * - q+  = q**((wm - wb)*dt)          <-- ** : quaternion product
		 * - ab+ = ab + ar                    <-- ar : random walk in acc bias with ar perturbation
		 * - wb+ = wb + wr                    <-- wr : random walk of gyro bias with wr perturbation
		 * - g+  = g                          <-- g  : gravity vector, constant but unknown
		 *
		 * \sa See the extense comments on move_func() in file robotInertial.cpp for algebraic details.
		 */
		class RobotInertial: public RobotAbstract {

			public:
				//				RobotInertial(MapAbstract & _map);
				RobotInertial(const map_ptr_t & _mapPtr);
				RobotInertial(const simulation_t dummy, const map_ptr_t & _mapPtr);

				~RobotInertial() {
				}
				/**
				 * Move one step ahead.
				 *
				 * This function predicts the robot state one step of length \a dt ahead in time,
				 * according to the control input \a control.x and the time interval \a control.dt.
				 * It updates the state and computes the convenient Jacobian matrices.
				 *
				 * This motion model is driven by IMU measurements and random perturbations, and defined by:
				 *
				 * - The state vector, x = [p q v ab wb g] , of size 19.
				 *
				 * - The transition equation x+ = move(x,u), with u = [am, wm, ar, wr] the control impulse, is decomposed as:
				 * - p+  = p + v*dt
				 * - v+  = v + R(q)*(am - ab) + g     <-- am and wm: IMU measurements
				 * - q+  = q**((wm - wb)*dt)          <-- ** : quaternion product
				 * - ab+ = ab + ar                    <-- ar : random walk in acc bias with ar perturbation
				 * - wb+ = wb + wr                    <-- wr : random walk of gyro bias with wr perturbation
				 * - g+  = g                          <-- g  : gravity vector, constant but unknown
				 *
				 * \sa See the extense comments on move_func() in file robotInertial.cpp for algebraic details.
				 */
				void move_func(const vec & _x, const vec & _u, const vec & _n, double _dt, vec & _xnew,
				    mat & _XNEW_x, mat & _XNEW_pert);

				virtual void move_func() {
					vec x = state.x();
					vec n = perturbation.x();
					vec xnew;
					move_func(x, control, n, dt_or_dx, x, XNEW_x, XNEW_pert);
					state.x() = xnew;
				}

				static size_t size() {
					return 19;
				}

				static size_t size_control() {
					return 6;
				}

				static size_t size_perturbation() {
					return 12;
				}

				virtual size_t mySize() {return size();}
				virtual size_t mySize_control() {return size_control();}
				virtual size_t mySize_perturbation() {return size_perturbation();}


			protected:
				/**
				 * Split state vector.
				 *
				 * Extracts \a p, \a q, \a v, \a ab, \a wb and \a g from the state vector, \a x = [\a p, \a q, \a v, \a ab, \a wb, \a g].
				 * \param p the position
				 * \param q the quaternion
				 * \param v the linear velocity
				 * \param ab the accelerometer bias
				 * \param wb the gyrometer bias
				 * \param g the gravity vector
				 */
				template<class Vx, class Vp, class Vq, class Vv, class Vab, class Vwb, class Vg>
				inline void splitState(const Vx & x, Vp & p, Vq & q, Vv & v, Vab & ab, Vwb & wb, Vg & g) {
					p = ublas::subrange(x, 0, 3);
					q = ublas::subrange(x, 3, 7);
					v = ublas::subrange(x, 7, 10);
					ab = ublas::subrange(x, 10, 13);
					wb = ublas::subrange(x, 13, 16);
					g = ublas::subrange(x, 16, 19);
				}


				/**
				 * Compose state vector.
				 *
				 * Composes the state vector with \a p, \a q, \a v, \a ab, \a wb and \a g, \a x = [\a p, \a q, \a v, \a ab, \a wb, \a g].
				 * \param p the position
				 * \param q the quaternion
				 * \param v the linear velocity
				 * \param ab the accelerometer bias
				 * \param wb the gyrometer bias
				 * \param g the gravity vector
				 */
				template<class Vp, class Vq, class Vv, class Vab, class Vwb, class Vg, class Vx>
				inline void unsplitState(const Vp & p, const Vq & q, const Vv & v, const Vab & ab, const Vwb & wb, const Vg & g, Vx & x) {
					ublas::subrange(x, 0, 3) = p;
					ublas::subrange(x, 3, 7) = q;
					ublas::subrange(x, 7, 10) = v;
					ublas::subrange(x, 10, 13) = ab;
					ublas::subrange(x, 13, 16) = wb;
					ublas::subrange(x, 16, 19) = g;
				}


				/**
				 * Split control vector.
				 *
				 * Extracts noisy measurements \a am and \a wm, and perturbationa \a ar and \a wr, from the control vector.
				 * \param am the acceleration measurements, with noise
				 * \param wm the gyrometer measurements, with noise
				 * \param ar the accelerometer bias random walk noise
				 * \param wr the gyrometer bias random walk noise
				 */
				template<class Vu, class V>
				inline void splitControl(const Vu & u, V & am, V & wm) {
					am = project(u, ublas::range(0, 3));
					wm = project(u, ublas::range(3, 6));
				}

				/**
				 * Split perturbation vector.
				 *
				 * Extracts noisy measurements \a am and \a wm, and perturbationa \a ar and \a wr, from the control vector.
				 * \param am the acceleration measurements, with noise
				 * \param wm the gyrometer measurements, with noise
				 * \param ar the accelerometer bias random walk noise
				 * \param wr the gyrometer bias random walk noise
				 */
				template<class Vn, class V>
				inline void splitPert(const Vn & n, V & an, V & wn, V & ar, V & wr) {
					an = project(n, ublas::range(0, 3));
					wn = project(n, ublas::range(3, 6));
					ar = project(n, ublas::range(6, 9));
					wr = project(n, ublas::range(9, 12));
				}

			private:
				// Temporary members to accelerate Jacobian computation
				mat33 Idt; ///<       Temporary I*dt matrix
				mat33 Rold, Rdt; ///< Temporary rotation matrices
				mat44 QNEW_q; ///<    Temporary Jacobian matrix
				mat44 QNEW_qwdt; ///< Temporary Jacobian matrix
				mat43 QWDT_w; ///<    Temporary Jacobian matrix
				mat43 QNEW_w; ///<    Temporary Jacobian matrix
				mat34 VNEW_q; ///<    Temporary Jacobian matrix

		};

	}
}

#endif /* ROBOTINERTIAL_HPP_ */
