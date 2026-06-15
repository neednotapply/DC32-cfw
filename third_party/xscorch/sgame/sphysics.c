/* $Header: /fridge/cvs/xscorch/sgame/sphysics.c,v 1.70 2011-07-31 23:44:42 jacob Exp $ */
/*

   xscorch - sphysics.c       Copyright(c) 2000-2003 Justin David Smith
                              Copyright(c)      2011 Jacob Luna Lundberg
   justins(at)chaos2.org      http://chaos2.org/
   jacob(at)gnifty.net        http://www.gnifty.net/

   Scorched physics


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License ONLY.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*/
#include <assert.h>
#include <stdlib.h>

#include <sphysics.h>
#include <sconfig.h>
#include <sland.h>
#include <splayer.h>
#include <sshield.h>
#include <stankpro.h>

#include <sai/sai.h>
#include <sutil/srand.h>



/*

   WARNING

   You are about to be treated with some rather unpleasant code.
   I suggest turning back.  Now would be a good time.

*/



/*

   DESIGN PHILOSOPHY

   If you're not going to listen to reason, then you might as well listen
   to this instead.  Excerpt from README for CS47 Homework #9, 1999, by
   Justin David Smith.  Perhaps you will recognize the original reference:

      Imagine.  It's Australia.  You're flying a military helicopter low to
      the ground.  To the left, you see a herd of kangaroos.  You avoid
      them, knowing that they will give away your position.  But one of
      their herd spies you flying overhead.  Within seconds, the herd has
      dispersed to strategic locations nearby.  Seconds later, a round of
      Stinger missiles are incoming.  Seconds later, you are toast.

      We have only ourselves to blame for such a predicament.  After all, we
      all advocate code re-use.  Good object-oriented programming promotes
      code re-use.  And at some point, when the code was being written for
      kangaroos, some lazy programmer decided it would be a great idea to
      inherit from another class that had already been written -- so what if
      that other class was Human_Military_Infantry?  It's all the same
      really, just replace an icon and presto!  You have a kangaroo.  Makes
      sense in a twisted sort of way.  It's reassuring to know that the
      programmer that brought about your demise was only carrying out the
      principle of code re-use.

      Or maybe this is just a testament to why I should not write
      documentation on little to no sleep.  I always wondered how the man
      page to the feather command came about; now I think I know ...

*/



/*

   ACCOUNTING FOR VISCOSITY rho

   Consider the force equation for viscosity in Y direction, noting that
   viscosity will always act opposite to gravity.  We will also require
   initial conditions, i.e. y(0) = y0 and y'(0) = vy.  rho has units of
   N m/sec; define nu = rho / m, which has units of sec/m.

      F = -m G + rho dy/dt G
      d^2y/dt^2 = -G + nu G dy/dt
      d^2y/dt^2 - nu G dy/dt = -G
      y(t) = [ (e^(G nu t) - 1) (nu vy - 1) + G nu (nu y0 + t) ]/(G nu^2)

   Observe that this equation only holds if nu != 0.  In the case where
   nu == 0, we reduce to our original equations above.




   TRAJECTORIES BASED ON STARTING ANGLE

   THE BASIC CASE:  NO WIND
   -------------------------

   We start by considering a simple case. Assume we have one force G, acting
   downward.  Then if we want to determine the angle a, and velocity, v,
   needed to reach (x, y), we can use standard Ph1a knowledge to solve:
      x = vx t
      y = vy t - 1/2 G t^2

   If we assume a, and solve for v:
      x = vx t  -->  t = x / vx
      y = x vy/vx - 1/2 G x^2/vx^2
      y = x tan a - 1/2 G x^2/(v^2 cos^2 a)
      y - x tan a = -1/2 G x^2/(v^2 cos^2 a)
      v^2 = 1/2 G x^2/cos^2 a * 1/(x tan a - y)

   Observe we can only have a valid trajectory if (x tan a) > y.  Also, we
   can't have cos a == 0, which means a cannot be vertical.  In the case
   where a is vertical:

      1) x == 0, y > 0:  then power is governed by max height:
         dy/dt = 0 = v - G t
                 t = v/G
         y = v^2/G - 1/2 v^2/G
         v = sqrt(2 G y)

      2) y < 0:  Can choose a minimal power to reach the destination.
         However, this "minimal power" causes a trajectory which includes us
         in its path.  This scenario is not desirable.

      3) If x != 0, then we cannot reach the target at all.


   EXTENDED CASE:  CONSIDERING WIND
   ---------------------------------

   Assume we have two orthogonal forces, G (acting downward) and W (toward
   +x).  Then, if we want to determine the angle a, and velocity v, required
   to reach coordinates (x,y), we can use the two force equations to
   determine:
      x = vx t + 1/2 W t^2
      y = vy t - 1/2 G t^2

   We can solve for v using the first equation:
      v = (x - 1/2 W t^2) / (t cos a)
      y = v sin a t - 1/2 G t^2
        = (x - 1/2 W t^2) tan a - 1/2 G t^2

      y - x tan a = -1/2 t^2 (W tan a + G)
      t^2 = 2(x tan a - y) / (W tan a + G)
          = 2(x sin a - y cos a) / (W sin a + G cos a)

   Therefore, we can find v in terms of a (note, the angle parameter will be
   a free (independent) variable).  Observe that, we must have a valid
   (nonzero) flight time for the solution to be valid.

   The special cases are:

      1) Wsin(a) + Gcos(a) == 0  -- weapon will hit source eventually.
         We can transform this into the equivalent special case for
         gravity by rotating (90-a) degrees clockwise.  Then we can
         run the "vertical" special cases, from above, noting that:
            x' = x cos(90-a) - y sin(90-a)   [ sin(90-a) = cos(a)
            y' = x sin(90-a) + y cos(90-a)     cos(90-a) = sin(a) ]

      2) t^2 <= 0:  Treat this as an impossible case.

      3) cos a == 0:  If this occurs, then t^2 = 2 x/W.  Treat as impossible
         for the time being, I'm not sure how to deal with this case.


   REDUCING CASE IF W == 0
   ------------------------

   If W == 0, then observe that flight time and velocity reduce to:
      t^2 = 2(x sin a - y cos a) / (G cos a)
          = 2(x tan a - y) / G
      v^2 = x^2 / (t cos a)^2
          = 1/2 G x^2/cos^2 a * 1/(x tan a - y)

   Which is the case predicted above, without considering wind.  Also
   observe that the special cases properly reduce to the special cases
   without considering wind.  Therefore, we conclude we can use the more
   general mechanism to solve trajectories.


   TRAJECTORIES BASED ON PARABOLA HEIGHT

   THE BASIC CASE:  NO WIND
   -------------------------

   We start by considering a simple case. From the previous analysis, we
   know the following formulas hold if (x tan a) > y, and cos a != 0:

      x = vx t
      y = vy t - 1/2 G t^2
      v^2 = 1/2 G x^2/cos^2 a * 1/(x tan a - y)

   We also have a requirement on the height, h, of the parabola:

      dy/dt = 0 = vy - G th   -->  th = vy / G
      h = vy th - 1/2 G th^2
        = vy^2 / G - 1/2 G vy^2 / g^2
        = 1/2 vy^2 / G
      vy^2 = 2 G h
      v^2 sin^2 a = 2 G h

   We can combine the two equations to solve for a, as follows:

      (1/2 G x^2/cos^2 a * 1/(x tan a - y)) sin^2 a = 2 G h
      1/2 G x^2 tan^2 a = 2 G h (x tan a - y)
      x^2 tan^2 a = 4 h (x tan a - y)           <-- A = tan a
      x^2 A^2 - 4 h x A + 4 h y = 0
      A = (4 h x -+ sqrt(16 h^2 x^2 - 16 x^2 h y)) / (2 x^2)
        = 1/x (2 h -+ 2 sqrt(h^2 - h y))

   Note the implied restriction that h > y, which makes sense.  Perhaps more
   interesting is the -+ -- which one do we choose?  Well, I posit that we
   want to choose "+".  When y==0, this equation suggests a positive angle,
   and an angle of zero as the two solutions -- inspection quickly reveals
   that an angle of zero can only work with infinite power.  We conclude
   that we should choose the positive branch.

      a = arctan( 2 (h + sqrt(h^2 - h y)) / x )

   The power can now be determined through the usual mechanism.


   EXTENDED CASE:  CONSIDERING WIND
   ---------------------------------

   This case is damn near impossible to calculate.  For the time being, it
   is NOT SUPPORTED.  (I've tried it by hand, and mathematica can't solve the
   equation either.  Sorry.)


   WHAT TIMESTEP REACHES A DESTINATION?
   -------------------------------------

   Consider reaching the destination (x0, y0) (with the source at (0, 0)).
   Ignoring wind, the time that reaches the destination is governed by the
   equations:

      x0 = vx t0
      y0 = vy t0 - 1/2 G t0^2

   Both equations must be satisfied, therefore:

      1/2 G t0^2 - vy t0 + y0 = 0
      t0 = (vy -+ sqrt(vy^2 - 4 * 1/2 G * y0)) / G
         = 1/G (vy -+ sqrt(vy^2 - 2 G y0))

   We generally assume the apex was already reached, therefore

      t0 = 2 vy/G + 2/G * sqrt(vy^2 - 2 G y0)

*/



/***     Basic Physics     ***/



/* Calculation of the default gravity and velocity scaling, which allow a
   weapon at maximum power to cross the playing field in a reasonable amount
   of time:

   Given:
      T = maximum flight time (seconds) (T=5s)
      P = maximum power (e.g., P=1000)
      w = playing field width (pixels) (e.g., w=800)
      h = playing field height (pixels) (e.g. h=600)
      C = cycle time (currently C=50ms/step)

   Calculate:
      a = optimal angle for crossing playing field
      S = velocity scaling factor (pixels/power_unit)
      g = gravity (pixels/step/step)


   Let:
      T' = T / C  (maximum number of steps)
      v  = P * S  (velocity, pixels/step)

      v cos(a) = w / T'    (these follow from Ph1a)
      h = (v sin(a)) T'/2 - g T'^2/8
      0 = -g T'/2 + (v sin(a))
      v sin(a) = g T'/2

   So:
      h = g T'^2/8      (we can determine g!)
      g = 8 h / T'^2    (units of pixel/step/step)

   Now...
      tan(a) = (T' / w) (g T'/2)
             = (T' / w) (8 h / T'^2) (T'/2)
             = 4 h / w
      a = arctan(4 h / w)

   Once we have the optimal angle, we have v:
      v = w / (T' cos(a))


   The equations are:
      g = 8 h C^2 / T^2
      S = w C / (T P cos(a))
*/



sc_physics *sc_physics_new(void) {
/* sc_physics_new
   Create a new physics parametre structure.  */

   sc_physics *ph;         /* New structure */

   /* Allocate memory for the parametre structure */
   ph = (sc_physics *)malloc(sizeof(sc_physics));
   if(ph == NULL) return(NULL);

   /* Initialise the various parametres */
   ph->airviscosity = 0;
   ph->gravity = SC_PHYSICS_GRAVITY_DEF;
   ph->damping = SC_TRAJ_TUNNEL_DAMPING;
   ph->maxwind = SC_PHYSICS_WIND_DEF;
   ph->suspenddirt = 0;
   ph->tanksfall = 100;
   ph->dynamicwind = true;
   ph->bordersextend = SC_PHYSICS_BORDERS_DEF;
   ph->walls = SC_WALL_WRAP;

   /* Return the new structure */
   return(ph);

}



void sc_physics_free(sc_physics **ph) {
/* sc_physics_free
   Release the physics parametre structure.  */

   if(ph == NULL || *ph == NULL) return;
   free(*ph);
   *ph = NULL;

}



void sc_physics_init_game(sc_physics *ph) {
/* sc_physics_init_game
   There's nothing to do these days on game init.  */

   assert(ph != NULL);

}



void sc_physics_init_round(sc_physics *ph) {
/* sc_physics_init_round
   Initialize a round.  This usually involves setting up the default wind. */

   assert(ph != NULL);
   ph->curwind = (game_drand() * 2 - 1) * ph->maxwind;

}



void sc_physics_update_wind(sc_physics *ph) {
/* sc_physics_update_wind
   Update the wind; this is done only if the wind is dynamic.  Note that
   a delta is applied to the wind, to keep it from literally jumping from
   anywhere, to anywhere.  */

   assert(ph != NULL);
   if(ph->dynamicwind) do {
      /* Update the wind, making sure to stay within the valid range. */
      if(ph->curwind < -ph->maxwind) {
         ph->curwind += game_drand() * ph->maxwind * SC_PHYSICS_DELTA_WIND_MAX;
      } else if(ph->curwind > ph->maxwind) {
         ph->curwind -= game_drand() * ph->maxwind * SC_PHYSICS_DELTA_WIND_MAX;
      } else {
         ph->curwind += (game_drand() * 2 - 1) * ph->maxwind * SC_PHYSICS_DELTA_WIND_MAX * 2;
      }
   } while(ph->curwind < -ph->maxwind || ph->curwind > ph->maxwind);

}



/***     Trajectory Code      ***/



static inline void _sc_traj_reset(sc_trajectory *tr) {
/* _sc_traj_reset
   Resets a trajectory.  This erases any "victim" information, used by the
   AI code to figure out who the trajectory eventually hit.  This also clears
   the timestep and sets all XY coordinates to the center of the trajectory.
   This function updates the "tracked" parameters, but preserves the params
   that indicate the physical model (initial position, initial velocity, etc). */

   assert(tr != NULL);

   tr->victim = -1;
   tr->timestep = 0;
   tr->curx = tr->oldx = tr->lastx = tr->ctrx;
   tr->cury = tr->oldy = tr->lastx = tr->ctry;
   tr->dir = 0;

}



void sc_traj_reinitialize(sc_trajectory *tr) {
/* sc_traj_reinitialize
   Reinitialize a trajectory.  This sets the physics model parametres
   to their initial i* values.  This also resets all "tracked" parametres
   such as the victim, current timestep, and current XY coordinates.  */

   assert(tr != NULL);

   tr->ctrx = tr->_ictrx;
   tr->ctry = tr->_ictry;
   tr->velx = tr->_ivelx;
   tr->vely = tr->_ively;
   tr->accx = tr->_iaccx;
   tr->accy = tr->_iaccy;
   _sc_traj_reset(tr);

}



static inline double _sc_traj_height(double vely, double gravity) {
/* sc_traj_height
   Calculates height attained by this trajectory, dictated by equations:

      dy/dt = 0 = vy - G t
         vy = G t
          t = vy/G
          y = vy t - 1/2 G t^2
            = vy^2 / (2 G)      */

   /* Calculate velocity squared, and factor in the current gravity. */
   return(0.5 * SQR(vely) / gravity);

}



static inline void _sc_traj_copy_init_params(sc_trajectory *tr) {
/* sc_traj_copy_init_params
   Copies the parametres for the initial control equation.  */

   tr->_ictrx = tr->ctrx;
   tr->_ictry = tr->ctry;
   tr->_ivelx = tr->velx;
   tr->_ively = tr->vely;
   tr->_iaccx = tr->accx;
   tr->_iaccy = tr->accy;

}



static inline sc_trajectory *_sc_traj_new(const sc_config *c, const sc_player *p,
                                          sc_trajectory_type type, int flags,
                                          double centerx, double centery) {
/* sc_traj_new
   Create a new trajectory structure with "reasonable" defaults.  Note
   that "reasonable" is defined by the person who wrote most of this
   file, namely Me, Justin, the programmer of DHOOM.  You would be very
   well justified to question my idea of "reasonable" at this stage.

   The trajectory is initialized with the centerx,centery coordinates
   as its starting position.  It will use the config structure to setup
   initial acceleration; velocity is cleared.  The flags can default
   to SC_TRAJ_DEFAULT.  */

   sc_trajectory *tr;         /* Storage location for trajectory */

   assert(c != NULL);
   assert(p != NULL);

   /* Attempt to allocate the memory */
   tr = (sc_trajectory *)malloc(sizeof(sc_trajectory));
   if(tr == NULL) return(NULL);

   /* Configure the type */
   tr->type = type;

   /* Initialise origin coordinate */
   tr->ctrx = centerx;
   tr->ctry = centery;

   /* Initialise velocity */
   tr->velx = 0;
   tr->vely = 0;

   /* Initialise accelerations */
   tr->accx = (flags & SC_TRAJ_IGNORE_WIND) ? 0 : c->physics->curwind;
   tr->accy = c->physics->gravity;
   tr->oaccx = tr->accx;
   tr->oaccy = tr->accy;

   /* Initialise "current" state */
   _sc_traj_reset(tr);

   /* Set terminus step */
   tr->stopstep = (flags & SC_TRAJ_BRIEF ? SC_TRAJ_TIMEOUT / 3 : SC_TRAJ_TIMEOUT);

   /* Clear out landfall params */
   tr->landfall_x1 = c->fieldwidth;
   tr->landfall_x2 = 0;

   /* Make the player assignment */
   tr->player = p;

   /* Victim setup */
   tr->victim = -1;

   /* Return the new structure */
   return(tr);

}



sc_trajectory *sc_traj_new_velocities(const sc_config *c, const sc_player *p, int flags,
                                      double centerx, double centery,
                                      double velx, double vely) {
/* sc_traj_new_velocities
   Create a new trajectory with the given initial velocity, broken up
   into X and Y components.  Accelerations are inherited from the config
   structure.  This builds a FLIGHT trajectory.  */

   sc_trajectory *tr;         /* Storage for the new trajectory */

   assert(c != NULL);
   assert(p != NULL);

   /* Create the trajectory */
   tr = _sc_traj_new(c, p, SC_TRAJ_FLIGHT, flags, centerx, centery);
   if(tr == NULL) return(NULL);

   /* Calculate initial velocity */
   tr->velx = velx;
   tr->vely = vely;

   /* Copy init state over */
   _sc_traj_copy_init_params(tr);

   /* Return the trajectory */
   return(tr);

}



sc_trajectory *sc_traj_new_power_angle(const sc_config *c, const sc_player *p, int flags,
                                       double centerx, double centery,
                                       double power, double angle) {
/* sc_traj_new_power_angle
   Create a new trajectory with the given power (initial velocity) and
   angle.  WARNING, the power value must already be scaled to "proper" units
   before it is passed to this function!  The angle is a standard angle in
   degrees, with 0==right, 90==up.  This builds a FLIGHT trajectory.  */

   sc_trajectory *tr;         /* Storage for the new trajectory */

   assert(c != NULL);
   assert(p != NULL);

   /* Create the trajectory */
   tr = _sc_traj_new(c, p, SC_TRAJ_FLIGHT, flags, centerx, centery);
   if(tr == NULL) return(NULL);

   /* Calculate initial velocity */
   tr->velx = power * cos(angle * M_PI / 180);
   tr->vely = power * sin(angle * M_PI / 180);

   /* Copy init state over */
   _sc_traj_copy_init_params(tr);

   /* Return the trajectory */
   return(tr);

}



/* Forward declarations for processing roller and digger trajectories. */
static void _sc_traj_roller_fallthrough(const sc_config *c, sc_trajectory *tr, int flags);
static bool _sc_traj_digger_reselect_dir(const sc_config *c, sc_trajectory *tr, int flags);



bool sc_trajectory_convert(const sc_config *c, sc_trajectory *tr,
                           int flags, sc_trajectory_type type) {
/* sc_trajectory_convert
   Convert from one trajectory type to another. Not all possible conversions
   are supported right now.  This is generally used when a warhead in flight
   that contains a roller/digger impacts; the trajectory must be converted
   from flight mode to roller/digger as appropriate...

   The original trajectory is given in tr; the type to convert *to* is given
   in type.  The physics of the original trajectory will be converted over
   in tr itself.  If the conversion is illegal, this function returns false.  */

   int height;                /* Height of land */
   double slope;              /* Slope of nearby land */

   assert(c != NULL);
   if(tr == NULL) return(false);

   /* Figure out what conversion is occurring */
   switch(tr->type) {
   case SC_TRAJ_FLIGHT:
      /* Originally a flying weapon */
      switch(type) {
      case SC_TRAJ_FLIGHT:
         tr->ctrx = tr->curx;
         tr->ctry = tr->cury;
         break;

      case SC_TRAJ_ROLLER:
         /* Note that tr->lastx, tr->lasty indicate the last
            coordinate that was NOT an impact.  We should start
            the rolling from there, since tr->curx, tr->cury
            reflect the actual point of impact. */

         /* Also fail the conversion if we've hit a tank. */
         if(tr->victim >= 0) {
            return(false);
         }

         /* Update the static data based on current dynamic info */
         tr->type = SC_TRAJ_ROLLER;
         tr->ctrx = tr->lastx;
         tr->ctry = tr->lasty;

         /* Find the rough slope of the local land. */
         slope = sc_land_height(c->land, tr->ctrx - 1, tr->lasty + 1) -
                 sc_land_height(c->land, tr->ctrx + 1, tr->lasty + 1);
         slope /= 2.0;

         /* If the weapon velocity and the slope oppose, reverse it. */
         if(tr->velx != 0 && slope / tr->velx > 0) {
            tr->velx = -tr->velx;
         }

         /* When rolling, we don't keep track of vely. */
         tr->vely = 0;

         /* Reinitialise the weapon */
         _sc_traj_reset(tr);
         _sc_traj_roller_fallthrough(c, tr, flags);
         break;

      case SC_TRAJ_DIGGER:
         /* Update the static data based on current dynamic info */
         tr->type = SC_TRAJ_DIGGER;
         tr->ctrx = tr->curx;
         tr->ctry = tr->cury;
         height = sc_land_height(c->land, tr->ctrx, c->land->height);
         if(height > tr->ctry) tr->ctry = height;
         tr->vely = 0;

         /* Reinitialise the weapon */
         _sc_traj_reset(tr);
         _sc_traj_digger_reselect_dir(c, tr, flags);
         break;
      } /* End of flight->? case */
      break;

   case SC_TRAJ_ROLLER:
      /* Rollers cannot convert. */
      fprintf(stderr, "sc_trajectory_convert: Illegal conversion ignored.\n");
      return(false);

   case SC_TRAJ_DIGGER:
      /* Diggers cannot convert. */
      fprintf(stderr, "sc_trajectory_convert: Illegal conversion ignored.\n");
      return(false);
   }

   /* At this point, do sc_traj_run as if nothing ever happened... */
   return(true);

}



void sc_traj_landfall(sc_config *c, const sc_trajectory *tr) {
/* sc_traj_landfall
   Process landfall as a result of trajectory tr.  This is called at the
   end of tr's life usually, when we determine just what effect it had on
   the land it passed through.  Since trajectories do not generally do
   much damage to the land, we process all landfall and player falls at
   once; this code is not animated.  */

   assert(tr != NULL && c != NULL);
   if(tr->landfall_x1 <= tr->landfall_x2) {
      while(sc_land_drop_zone(c, c->land, tr->landfall_x1, tr->landfall_x2)) /* Just loop */;
      while(sc_player_drop_all(c)) /* Just loop */;
   }

}



void sc_traj_free(sc_trajectory **tr) {
/* sc_traj_free
   Releases a trajectory.  */

   if(tr == NULL || *tr == NULL) return;
   free(*tr);
   *tr = NULL;

}



static double _sc_traj_power(double gravity, double wind,
                             __libj_unused double visc,
                             double deltax, double deltay,
                             double angle) {
/* sc_traj_power
   Calculates power required for a trajectory to the specified coordinates,
   using a given angle, taking into account gravity and wind.  We assume the
   current player is at coordinates (0, 0).  Gravity value is always
   positive, and oriented downward.  Wind value is positive if the wind is
   directed to the right (+x direction).  This function returns a value less
   than zero if the trajectory could not be computed along the given angle. */

   double flighttime;/* Total flying time to reach destination */
   double flttime2;  /* Square of time required to reach target */
   double WSin_GCos; /* (W sin a + G cos a) */
   double velocity;  /* Real velocity rqd to reach (dx,dy) */
   double sina;      /* Sine of angle */
   double cosa;      /* Cosine of angle */
   double xprime;    /* Transformed X coordinate */
   double yprime;    /* Transformed Y coordinate */

   /* Calculate the relevant sines and cosines. */
   sina = sin(angle * M_PI / 180);
   cosa = cos(angle * M_PI / 180);

   /* Check if weapon will hit source eventually. */
   WSin_GCos = wind * sina + gravity * cosa;
   if(WSin_GCos == 0) {
      /* Transform so coords are in a vertical forcing field. */
      xprime = deltax * sina - deltay * cosa;
      yprime = deltax * cosa + deltay * sina;
      if(xprime != 0 || yprime <= 0) return(-1);
      return(sqrt(2 * WSin_GCos * yprime));
   } /* Are we firing on 1 dimension? */

   if(cosa == 0) {
      /* We don't know how to deal with this case. */
      return(-1);
   }

   /* Calculate the flying time. */
   flttime2 = 2 * (deltax * sina - deltay * cosa);
   flttime2 = flttime2 / WSin_GCos;
   if(flttime2 <= 0) {
      /* Flight time is impossible. */
      return(-1);
   } /* Is flight time valid? */
   flighttime = sqrt(flttime2);

   /* Calculate the real velocity. */
   velocity = (deltax - 0.5 * wind * flttime2);
   velocity = velocity / (flighttime * cosa);

   /* Sanity check; velocity cannot be negative. */
   if(velocity < 0) {
      return(-1);
   }

   /* Return the power value */
   return(velocity);

}



sc_trajectory *sc_traj_new_dest_angle(const sc_config *c, const sc_player *p, int flags,
                                      double centerx, double centery,
                                      double deltax, double deltay,
                                      double angle, double maxpower) {
/* sc_traj_new_dest_angle
   Create a new trajectory with the given power (initial velocity) and
   angle.  The destination coordinate is specified as a delta, i.e. it is
   specified _relative_ to the center coordinate.  The angle is is it was
   with new_power_angle().  */

   sc_trajectory *tr;         /* Storage for the new trajectory */
   double height;             /* Maximum height of arc */
   double power;              /* Calculated power rating */
   double wind;               /* Wind value to use */
   double visc;               /* Viscosity to use */

   /* We must have a valid config */
   if(c == NULL) return(NULL);

   /* Create the trajectory */
   tr = _sc_traj_new(c, p, SC_TRAJ_FLIGHT, flags, centerx, centery);
   if(tr == NULL) return(NULL);

   /* Get wind and viscosity ratings */
   wind = (flags & SC_TRAJ_IGNORE_WIND) ? 0 : c->physics->curwind;
   visc = (flags & SC_TRAJ_IGNORE_VISC) ? 0 : c->physics->airviscosity;

   /* Calculate an expected power rating */
   power = _sc_traj_power(c->physics->gravity, wind, visc, deltax, deltay, angle);
   if((power < 0 || power > maxpower) && !(flags & SC_TRAJ_RESTRICTIVE)) {
      /* If wrapping walls, then we can try two more trajectories */
      if(c->physics->walls == SC_WALL_WRAP) {
         power = _sc_traj_power(c->physics->gravity, wind, visc, deltax + c->fieldwidth, deltay, angle);
         if(power < 0 || power > maxpower) {
            power = _sc_traj_power(c->physics->gravity, wind, visc, deltax - c->fieldwidth, deltay, angle);
         } /* Tried a third trajectory */
      } /* Do we have wrapping boundaries? */
   } /* Can we try other trajectories? */

   /* Was the power rating valid? */
   if(power < 0 || power > maxpower) {
      /* Trajectory cannot be computed */
      free(tr);
      return(NULL);
   }

   /* Calculate initial velocity */
   tr->velx = power * cos(angle * M_PI / 180);
   tr->vely = power * sin(angle * M_PI / 180);

   /* Make sure we won't hit the upper boundary */
   if(c->physics->walls != SC_WALL_NONE && c->physics->walls != SC_WALL_WRAP) {
      height = sc_traj_height(tr) + tr->ctry + SC_TRAJ_THRESHOLD;
      if(height >= c->fieldheight) {
         /* Weapon would have hit the ceiling.  Invalid. */
         free(tr);
         return(NULL);
      } /* Did we hit ceiling? */
   } /* Do we have boundaries? */

   /* Copy init state over */
   _sc_traj_copy_init_params(tr);

   /* Return the trajectory */
   return(tr);

}



static double _sc_traj_angle(__libj_unused double gravity, __libj_unused double wind,
                             __libj_unused double visc,
                             double deltax, double deltay,
                             double height) {
/* sc_traj_angle
   Calculates angle required for a trajectory to the specified coordinates,
   while attaining a given maximum height, taking into account gravity.
   WARNING, this function is unable to consider the contribution from wind.
   We assume the current player is at coordinates (0, 0).  Gravity value is
   always positive, and oriented downward.  Wind value is positive if the
   wind is directed to the right (+x direction).  This function returns a
   value less than zero if the trajectory could not be computed that would
   reach the given height. */

   double yslope;    /* Calculation for y-slope */
   double angle;     /* Expected firing angle */

   /* Calculate value in the inner sqrt */
   yslope = height * (height - deltay);
   if(yslope < 0) {
      /* Height is less than target Y */
      return(-1);
   }

   /* Calculate the remainder of the Y slope */
   yslope = 2 * (height + sqrt(yslope));

   /* Calculate target angle */
   angle = atan2(yslope, deltax) * 180 / M_PI;
   if(angle < 0) angle += 360;

   /* Return the angle (if it is valid) */
   if(angle > 180) return(-1);
   return(angle);

}



static double _sc_traj_timesteps(const sc_trajectory *tr, double y0) {
/* sc_traj_timesteps
   Estimate the number of timesteps to reach coordinate y0, given the
   y velocity and acceleration defined in the trajectory.  Returns -1
   if y0 is unattainable.  */

   double root;      /* Calculation for sqrt() part */
   double time;      /* Timestep this occurs at */

   assert(tr != NULL);

   /* Calculate value in the inner sqrt */
   root = SQR(tr->vely) - 2 * tr->accy * y0;
   if(root < 0) {
      return(-1);
   }

   /* Calculate the timesteps */
   time = 1 / tr->accy * (tr->vely + sqrt(root));
   return(time);

}



sc_trajectory *sc_traj_new_dest_height(const sc_config *c, const sc_player *p, int flags,
                                       double centerx, double centery,
                                       double deltax, double deltay,
                                       double height) {
/* sc_traj_new_dest_height
   Create a new trajectory with the given power (initial velocity) and
   angle.  The destination coordinate is specified as a delta, i.e. it is
   specified _relative_ to the center coordinate.  The height is specified
   relative to center coordinate, and we must have height > deltay.  */

   sc_trajectory *tr;         /* Storage for the new trajectory */
   double angle;              /* Calculated angle to fire at */
   double power;              /* Calculated power rating */
   double wind;               /* Wind value to use */
   double visc;               /* Viscosity to use */

   /* We must have a valid config */
   if(c == NULL) return(NULL);

   /* Try to get wind and viscosity models */
   wind = (flags & SC_TRAJ_IGNORE_WIND) ? 0 : c->physics->curwind;
   visc = (flags & SC_TRAJ_IGNORE_VISC) ? 0 : c->physics->airviscosity;

   /* Create the trajectory */
   tr = _sc_traj_new(c, p, SC_TRAJ_FLIGHT, flags, centerx, centery);
   if(tr == NULL) return(NULL);

   /* Calculate an expected firing angle */
   angle = _sc_traj_angle(c->physics->gravity, wind, visc, deltax, deltay, height);
   if(angle < 0) {
      /* Trajectory cannot be computed */
      free(tr);
      return(NULL);
   }

   /* Calculate an expected power rating */
   power = _sc_traj_power(c->physics->gravity, wind, visc, deltax, deltay, angle);
   if(power < 0) {
      /* Trajectory cannot be computed */
      free(tr);
      return(NULL);
   }

   /* Calculate initial velocity */
   tr->velx = power * cos(angle * M_PI / 180);
   tr->vely = power * sin(angle * M_PI / 180);

   /* Must we terminate at destination? */
   if(flags & SC_TRAJ_TERMINUS) {
      tr->stopstep = _sc_traj_timesteps(tr, deltay);
   }

   /* Copy init state over */
   _sc_traj_copy_init_params(tr);

   /* Return the trajectory */
   return(tr);

}



/***     Trajectory calculations    ***/



/* This structure is used to store current tracking progress. */
typedef struct _sc_trajectory_data {
   double stepx;  /* Velocity X, per iteration (not in "std" units) */
   double stepy;  /* Velocity Y, per iteration (not in "std" units) */
} sc_trajectory_data;



/***  Detect if we PASSED anything (boundaries)  ***/



static inline sc_trajectory_result _sc_traj_pass_wall_none(const sc_config *c,
                                                           const sc_trajectory *tr) {
/* sc_traj_pass_wall_none
   Boundary case for no walls.  */

   int extend;                /* How far do the borders extend? */

   assert(c != NULL && tr != NULL);

   /* How far do the borders extend? */
   extend = c->physics->bordersextend;

   /* Check if we hit the ground. */
   if(rint(tr->cury) < 0) {
      return(SC_TRAJ_IMPACT_GROUND);
   }

   /* Weapon should no longer be tracked if we go too far away. */
   if(tr->curx < -extend || tr->curx > c->fieldwidth + extend) {
      return(SC_TRAJ_SIZZLE);
   }

   /* Nothing interesting occurred */
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_pass_wall_wrap(const sc_config *c,
                                                           sc_trajectory *tr) {
/* sc_traj_pass_wall_wrap
   Boundary case for horizontal wrapping.  */

   assert(c != NULL && tr != NULL);

   /* Check if we hit the ground or went too high */
   if(rint(tr->cury) < 0) {
      return(SC_TRAJ_IMPACT_GROUND);
   }

   /* Implement weapon wrapping on horizontal boundaries */
   while(rint(tr->curx) >= c->fieldwidth) {
      tr->curx -= c->fieldwidth;
      tr->ctrx -= c->fieldwidth;
   } /* Adjusting X coordinate */
   while(rint(tr->curx) < 0) {
      tr->curx += c->fieldwidth;
      tr->ctrx += c->fieldwidth;
   } /* Adjusting Y coordinate */

   /* Not much occurred ... */
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_pass_wall_concrete(const sc_config *c,
                                                               const sc_trajectory *tr) {
/* sc_traj_pass_wall_concrete
   Boundary case for a good ol' solid concrete box.  */

   assert(c != NULL && tr != NULL);

   /* Check if we hit anything. */
   if(rint(tr->curx) < 0 || rint(tr->curx) >= c->fieldwidth) {
      return(SC_TRAJ_IMPACT_WALL);
   }
   if(rint(tr->cury) >= c->fieldheight) {
      return(SC_TRAJ_IMPACT_WALL);
   }
   if(rint(tr->cury) < 0) {
      return(SC_TRAJ_IMPACT_GROUND);
   }

   /* Not much occurred ... */
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_pass_wall_elastic(const sc_config *c, sc_trajectory *tr,
                                                              sc_trajectory_data *t, double elasticity) {
/* sc_traj_pass_wall_elastic
   Boundary case for elastic walls, with elasticity given (values < 1, wall
   absorbs some velocity, > 1, wall adds more velocity).  */

   assert(c != NULL && tr != NULL && t != NULL);
   assert(elasticity > 0);

   /* Check if we hit sidewalls. */
   if(rint(tr->curx) < 0) {
      tr->curx =  0;
      tr->ctrx =  tr->curx;
      tr->ctry =  tr->cury;
      tr->velx = -sc_traj_get_velocity_x(tr) * elasticity;
      tr->vely =  sc_traj_get_velocity_y(tr);
      tr->stopstep -= tr->timestep;
      tr->timestep  = 0;
      tr->finalstep = 0;
      t->stepx  =  fabs(t->stepx) * elasticity;
      t->stepy *=  elasticity;
   } else if(rint(tr->curx) >= c->fieldwidth) {
      tr->curx =  c->fieldwidth - 1;
      tr->ctrx =  tr->curx;
      tr->ctry =  tr->cury;
      tr->velx = -sc_traj_get_velocity_x(tr) * elasticity;
      tr->vely =  sc_traj_get_velocity_y(tr);
      tr->stopstep -= tr->timestep;
      tr->timestep  = 0;
      tr->finalstep = 0;
      t->stepx  = -fabs(t->stepx) * elasticity;
      t->stepy *=  elasticity;
   }

   /* Check if we hit ground or ceiling. */
   if(rint(tr->cury) < 0) {
      /* Rollers need to feel solid earth under their feet (otherwise infinite loop). */
      if(tr->type == SC_TRAJ_ROLLER)
         return(SC_TRAJ_IMPACT_GROUND);

      tr->cury =  0;
      tr->ctry =  tr->cury;
      tr->ctrx =  tr->curx;
      tr->vely = -sc_traj_get_velocity_y(tr) * elasticity;
      tr->velx =  sc_traj_get_velocity_x(tr);
      tr->stopstep -= tr->timestep;
      tr->timestep  = 0;
      tr->finalstep = 0;
      t->stepy  =  fabs(t->stepy) * elasticity;
      t->stepx *=  elasticity;
   } else if(rint(tr->cury) >= c->fieldheight) {
      tr->cury =  c->fieldheight - 1;
      tr->ctry =  tr->cury;
      tr->ctrx =  tr->curx;
      tr->vely = -sc_traj_get_velocity_y(tr) * elasticity;
      tr->velx =  sc_traj_get_velocity_x(tr);
      tr->stopstep -=  tr->timestep;
      tr->timestep  =  0;
      tr->finalstep =  0;
      t->stepy  = -fabs(t->stepy) * elasticity;
      t->stepx *=  elasticity;
   }

   /* Not much occurred ... */
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_traj_pass_wall(const sc_config *c, sc_trajectory *tr, sc_trajectory_data *t) {
/* sc_traj_pass_wall
   Determine whether we hit a wall with the weapon at (t).  Action depends
   on the wall type.  This function may choose to detonate here.  Returns
   NO_ACTION if the weapon status hasn't changed lately.  */

   assert(c != NULL && tr != NULL && t != NULL);

   switch(c->physics->walls) {
      case SC_WALL_NONE:
      case SC_WALL_RANDOM:
         return(_sc_traj_pass_wall_none(c, tr));

      case SC_WALL_WRAP:
         return(_sc_traj_pass_wall_wrap(c, tr));

      case SC_WALL_CONCRETE:
         return(_sc_traj_pass_wall_concrete(c, tr));

      case SC_WALL_PADDED:
         return(_sc_traj_pass_wall_elastic(c, tr, t, SC_PHYSICS_ELASTIC_PADDED));

      case SC_WALL_RUBBER:
         return(_sc_traj_pass_wall_elastic(c, tr, t, SC_PHYSICS_ELASTIC_RUBBER));

      case SC_WALL_SPRING:
         return(_sc_traj_pass_wall_elastic(c, tr, t, SC_PHYSICS_ELASTIC_SPRING));
   }
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_traj_pass_shield(const sc_config *c, sc_trajectory *tr,
                                                 int flags, const sc_trajectory_data *t) {
/* sc_traj_pass_shield
   Determine whether we are passing through a tank's shields with the
   weapon at (t).  The weapon might explode, or it might sizzle, or it
   might pass right on through... */

   const sc_player *p; /* Player structure */
   int i;              /* Index thru players */

   assert(c != NULL && tr != NULL && t != NULL);

   /* Iterate through list of players */
   for(i = c->numplayers - 1; i >= 0; --i) {
      p = c->players[i];
      if(SC_PLAYER_IS_ALIVE(p) &&
            sc_shield_would_impact(c, tr->player, p, flags, tr->lastx, tr->lasty, tr->curx, tr->cury)) {
         /* Weapon hit the shield */
         tr->victim = i;
         return(SC_TRAJ_IMPACT_SHIELD);
      } /* Player was shielded and wasn't dead.  Bummer.  :) */
   } /* Loop through players */

   /* No tanks were hit */
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_pass(const sc_config *c, sc_trajectory *tr,
                                                 int flags, sc_trajectory_data *t) {
/* sc_traj_pass
   Returns results if weapon passed through a non-pixel boundary.
   It might be modified as needed to contain new trajectory information.
   NOTE: These functions must be idempotent! */

   sc_trajectory_result result;/* Result from sc_traj_hit_tank. */

   assert(c != NULL && tr != NULL && t != NULL);

   /* Check for horizontal/vertical wall boundaries */
   result = _sc_traj_pass_wall(c, tr, t);
   if(result != SC_TRAJ_CONTINUE) return(result);

   /* Check if we hit a tank's shields */
   if(!(flags & SC_TRAJ_IGNORE_TANK)) {
      result = _sc_traj_pass_shield(c, tr, flags, t);
      if(result != SC_TRAJ_CONTINUE) return(result);
   }

   /* We hit nothing at this coordinate */
   return(SC_TRAJ_CONTINUE);

}



/***  Detect if we HIT anything (impact)  ***/



static sc_trajectory_result _sc_traj_hit_tank(const sc_config *c, sc_trajectory *tr) {
/* sc_traj_hit_tank
   Determine whether we hit a tank with the weapon at (t).
   The weapon might explode, or it might pass right on through... */

   const sc_player *p; /* Player structure */
   int tx, ty;         /* Weapon current X, Y */
   int i;              /* Index thru players */

   assert(c != NULL && tr != NULL);

   /* Determine current weapon position */
   tx = rint(tr->curx);
   ty = rint(tr->cury);

   /* Iterate through list of players */
   for(i = c->numplayers - 1; i >= 0; --i) {
      p = c->players[i];
      /* We use !p->dead here because missiles should still hit the tank
         until it is actually removed from the playing field... */
      if(!p->dead && sc_player_would_impact(c, p, tx, ty)) {
         /* Weapon hit the tank itself */
         tr->victim = i;
         return(SC_TRAJ_IMPACT_TANK);
      } /* Hit a tank that was still around */
   } /* Loop through players */

   /* No tanks were hit */
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_traj_hit_land(const sc_config *c, sc_trajectory *tr, int flags) {
/* sc_traj_hit_ground
   Returns SC_TRAJ_IMPACT_LAND if the weapon hit the ground.
   Otherwise, the value SC_TRAJ_CONTINUE will be returned.
   On detonation, use the current coordinates of (t) to
   determine center of the explosion.  */

   int tx;           /* current X coordinate of weapon */
   int ty;           /* current Y coordinate of weapon */
   double velx;      /* revisionist X velocity of weapon */
   double vely;      /* revisionist Y velocity of weapon */
   int *lp;          /* Pointer into land structure */

   assert(c != NULL && tr != NULL);

   /* Determine current weapon coordinates */
   tx = rint(tr->curx);
   ty = rint(tr->cury);

   /* Is weapon currently on the screen? */
   if(sc_land_translate_xy(c->land, &tx, &ty)) {
      lp = SC_LAND_XY(c->land, tx, ty);
      if(!SC_LAND_IS_SKY(*lp)) {
         /* WE HAVE IMPACTED.  Either we do not allow tunneling, in
            which case the weapon must impact.  Or we do allow some
            degree of tunneling, in which case if the weapon allows
            tunnelling and we do not have contact triggers, then...
            well... sucks for the player... */
         /* TEMP - One fix for our chronic roller problems is to
            disallow tunneling here for all but flying weapons.
            I am quite happy doing this to rollers but what will it
            do to groundhogs?  -JTL */
         if((flags & SC_TRAJ_TUNNELING) && (SC_TRAJ_IS_FLYING(tr))) {
            /* Update stepping for this interpolation pass */
            /* Actually, we CANNOT update velocities during interpolation.
               All parameters are already in the context of the next interp
               vertex so we can at best revise the course at that point,
               and hope that is sufficient. */

            /* Update current real velocity of weapon */
            velx = sc_traj_get_velocity_x(tr) * c->physics->damping;
            vely = sc_traj_get_velocity_y(tr) * c->physics->damping;
            if(SQR(velx) + SQR(vely) >= SQR(SC_TRAJ_TUNNEL_MIN_VEL)) {
               /* Still moving fast enough not to detonate */
               /* NOTE - We used to revise only if TRAJ_IS_FLYING... */
               sc_traj_revise_velocity(tr, velx, vely);
               sc_traj_revise_acceleration(tr, 0, 0);
               return(SC_TRAJ_CONTINUE);
            }
         }

         /* Not tunnelling, or no longer able to tunnel. Abort. */
         return(SC_TRAJ_IMPACT_LAND);
      }
   } /* If weapon currently on the screen? */

   /* Still not much to do ... */
   /* Be sure to restore any old accelerations since we're in sky */
   if(SC_TRAJ_IS_FLYING(tr)) sc_traj_restore_acceleration(tr);
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_hit(const sc_config *c, sc_trajectory *tr, int flags,
                                                const sc_trajectory_data *t) {
/* sc_traj_hit
   Returns results if weapon tried to occupy the same pixel as something.
   It might be modified as needed to contain new trajectory information.
   NOTE: These functions do not have to be idempotent. */

   sc_trajectory_result result;/* Result from sc_traj_hit_tank. */

   assert(c != NULL && tr != NULL && t != NULL);

   /* Check if we hit a tank itself. */
   if(!(flags & SC_TRAJ_IGNORE_TANK)) {
      result = _sc_traj_hit_tank(c, tr);
      if(result != SC_TRAJ_CONTINUE) return(result);
   }

   /* Check if we hit something that is not passable. */
   if(!(flags & SC_TRAJ_IGNORE_LAND)) {
      result = _sc_traj_hit_land(c, tr, flags);
      if(result != SC_TRAJ_CONTINUE) return(result);
   }

   /* We hit nothing at this coordinate */
   return(SC_TRAJ_CONTINUE);

}



static inline sc_trajectory_result _sc_traj_pass_or_hit(const sc_config *c, sc_trajectory *tr,
                                                        int flags, sc_trajectory_data *t) {
/* sc_traj_pass_or_hit
   Returns the result of either pass-through or impact codes.
   This function is NOT idempotent; do not use it in flight
   code or other code where you cannot safely call hit() more
   than once on the same screen pixel.  */

   sc_trajectory_result result;/* Result of pass/hit functions */

   result = _sc_traj_pass(c, tr, flags, t);
   if(result == SC_TRAJ_CONTINUE) {
      result = _sc_traj_hit(c, tr, flags, t);
   }
   return(result);

}



/***  Step Between Successive Interpolation Points  ***/



static sc_trajectory_result _sc_traj_traverse_path(sc_config *c, sc_trajectory *tr, int flags,
                                                   sc_trajectory_action action, void *data) {
/* sc_traj_traverse_path
   This function follows a weapon, from source (x,y) to its (intended)
   destination of (x+vx,y+vy).  Compensations in velocity are automatically
   made if the weapon bounces off a wall, etc, until weapon has travelled
   for the proper amount of time.  On return, *wp will contain a new
   position (or detonation position) and a velocity.

   Note that this function WILL track subpixel movements, so that shield
   boundary detection etc. work properly.  */

   sc_trajectory_result result;/* Tracking result for this function */
   sc_trajectory_data t;   /* Data for tracking (passed to subfunctions) */
   int deltax;             /* Change in X, round up to nearest int */
   int deltay;             /* Change in Y, round up to nearest int */
   int numsteps;           /* Number of steps to take */
   int iterator;           /* Iterator variable */

   assert(c != NULL && tr != NULL);

   /* ENTERING THIS FUNCTION:
         tr->curx, tr->cury reflect the interpolation point we are
            trying to reach.  If we have a smooth flight, the weapon
            will effectively be at this interpolation point at the
            end of the function.
         tr->oldx, tr->oldy reflect the interpolation point we were
            at when we entered this function.  This is the point we
            are moving away from.
         tr->lastx, tr->lasty reflect the last coordinate (rounded
            to the nearest pixel) that was actually TESTED for an
            impact or boundary crossing.  For determining if we
            have already checked a pixel for impact (and possibly
            already cleared it), we use lastx,y.  Also, to determine
            if the weapon just crossed a shield boundary, we rely
            on lastx,y.

      DURING THIS FUNCTION:
         tr->curx, tr->cury are updated to reflect the coordinate
            that is currently being tested.  When action is called,
            curx,y will reflect the current weapon position, not the
            intended final destination.  If the weapon impacts before
            it reaches the control point, then curx,y will reflect
            the point of impact.
         tr->lastx, tr->lasty are maintained to always reflect the
            last coordinate that was actually tested.
    */

   /* Populate data in t, with initial position and "real" velocity */
   t.stepx = tr->curx - tr->oldx;
   t.stepy = tr->cury - tr->oldy;
   tr->curx = tr->oldx;
   tr->cury = tr->oldy;

   /* Determine the distance travelled along X and Y (round up,
      so worst case scenario we double-check a discrete point,
      as opposed to accidentally skipping discrete points).  */
   deltax = ceil(fabs(t.stepx));
   deltay = ceil(fabs(t.stepy));

   /* WARNING:  We can ONLY abort if there was NO step whatsoever.
      If we tracked subpixel movement (e.g. the initial and final
      coordinates round to the same pixel), we must still go through
      all the motions, otherwise we might pass through boundaries
      without realizing it... */

   /* Sanity check; if we're not moving, then abort. */
   if(deltax == 0 && deltay == 0) return(SC_TRAJ_CONTINUE);

   /* Which axis changes more?  Label that axis the "primary" axis,
      which will determine the number of steps we need to take.  */
   if(deltax > deltay) {
      /* Primary traversal axis is X */
      numsteps = deltax;
   } else {
      /* Primary traversal axis is Y */
      numsteps = deltay;
   } /* Which axis is the primary axis? */

   /* Calculate new trajectory "velocities", which are the distance to
      travel in one iterative step.  The step velocities are therefore
      not in the same units as the normal velocities.  */
   t.stepx /= numsteps;
   t.stepy /= numsteps;

   /* Track along <numsteps> points, approximating a line from (x,y)
      to (curx,cury) -- note, that this line might change if the weapon
      is deflected from its original course.  */
   result = SC_TRAJ_CONTINUE;
   iterator = numsteps;
   while(iterator > 0 && result == SC_TRAJ_CONTINUE) {
      /* Make sure curx, cury are up-to-date.  If anything bails here, then
         they will reflect the point of impact (point of contention).  The
         old behaviour was inconsistent about what curx,y would reflect; for
         hit() it would reflect the last "safe" coordinate, but for the
         action handler it'd reflect the point of impact.  */

      /* Check if we passed through any boundaries.  Note that _sc_traj_pass
         is allowed to modify the trajectory, in the case where we pass an
         elastic wall boundary.  We must always perform the boundary-pass
         check.  */
      result = _sc_traj_pass(c, tr, flags, &t);
      if(result == SC_TRAJ_CONTINUE &&
        !(rint(tr->lastx) == rint(tr->curx) && rint(tr->lasty) == rint(tr->cury))) {
         /* Check if we hit anything.  None of these calls modify the
            trajectory, so we don't need to reassign any data here.
            Note that _sc_traj_hit is NOT idempotent; as a result, we
            must call it at most once per pixel, hence the long
            test to make sure we are not actually in the same pixel.  */
         result = _sc_traj_hit(c, tr, flags, &t);
      }

      /* Well, what happened with the impact tests? */
      if(result == SC_TRAJ_CONTINUE) {
         /* Weapon did not hit something; continue ... */
         if(action != NULL) {
            result = action(c, tr, data);
         } /* Is user using an action function? */
         if(result == SC_TRAJ_CONTINUE) {
            /* If we're here, then everyone signalled all-clear. */
            tr->lastx = tr->curx;
            tr->lasty = tr->cury;
            tr->curx += t.stepx;
            tr->cury += t.stepy;
         }
      } /* Did we hit something? */
      --iterator;    /* Next please */
   } /* While we are still following along the path... */

   /* Weapon will contain the last coordinates that were tracked.
      This means if it exploded, it will contain the coordinates
      of the explosion.  Otherwise, its velocity and position will
      be the new values (before gravity is considered).  */

   /* Return our status. */
   return(result);

}



static void _sc_traj_roller_setup(sc_trajectory *tr, sc_trajectory_data *t,
                                  double startx, double starty,
                                  double deltax, double deltay) {
/* sc_traj_roller_setup
   Setup a trajectory to attempt a roll in the indicated direction
   relative to startx, starty.  */

   assert(tr != NULL && t != NULL);

   tr->curx = startx + deltax;
   tr->cury = starty + deltay;
   t->stepx = deltax;
   t->stepy = deltay;

}



static void _sc_traj_roller_restore(sc_trajectory *tr, double startx, double starty) {
/* sc_traj_roller_restore
   Restore the trajectory's position.  */

   assert(tr != NULL);

   tr->curx = startx;
   tr->cury = starty;

}



static bool _sc_traj_roller_clear(const sc_config *c, sc_trajectory *tr, int flags,
                                  double deltax, double deltay) {
/* sc_traj_roller_clear
   This takes a trajectory, and plants it at (curx + deltax),
   (cury + deltay).  It then checks to see if the trajectory
   impacts anything at the specified location.  If NOT, then this
   function returns true, indicating that the specified position
   is clear.  This function will restore the trajectory's
   original position regardless of return value; the trajectory
   is effectively constant.  */

   sc_trajectory_data t;   /* Trajectory steps */
   double origx;           /* Original X position */
   double origy;           /* Original Y position */

   /* Sanity checks */
   assert(c != NULL && tr != NULL);

   /* Save the original X position */
   origx = tr->curx;
   origy = tr->cury;

   /* Setup the temporary trajectory and step data */
   _sc_traj_roller_setup(tr, &t, origx, origy, deltax, deltay);

   /* Check for impact in the specified location. */
   if(!SC_TRAJ_IS_IMPACT(_sc_traj_pass_or_hit(c, tr, flags, &t))) {
      /* We didn't hit! */
      _sc_traj_roller_restore(tr, origx, origy);
      return(true);
   } else {
      /* We impacted something; specified location is not clear */
      _sc_traj_roller_restore(tr, origx, origy);
      return(false);
   }

}



static void _sc_traj_roller_fallthrough(const sc_config *c, sc_trajectory *tr, int flags) {
/* sc_traj_roller_fallthrough
   If falling, then we are allowed to reverse direction once we land.
   We don't care about impact type here, because we assume when this
   is called we've already checked for candidate detonations.  This
   function will adjust the velocity vector to indicate a direction
   reversal, if applicable.  At the end, the trajectory remains in the
   same location it was in on entry.  */

   /* Sanity checks */
   assert(c != NULL && tr != NULL);

   if(SC_PHYSICS_DEBUG_ROLLER) {
      printf("roller fallthru %8p  at %16g %16g  land is %4d\n",
             (void *)tr, tr->curx, tr->cury,
             sc_land_height(c->land, rint(tr->curx), c->land->height));
   }

   /* First, attempt a downward motion */
   if(_sc_traj_roller_clear(c, tr, flags, 0, -1)) {
      if(SC_PHYSICS_DEBUG_ROLLER) {
         printf("roller fallthru %8p  descent available\n", (void *)tr);
      }
      return;
   }

   /* That didn't work.  Follow the current velocity. */
   if(_sc_traj_roller_clear(c, tr, flags, SGN(tr->velx), 0)) {
      if(SC_PHYSICS_DEBUG_ROLLER) {
         printf("roller fallthru %8p  momentum continuing in direction %+d available\n",
                (void *)tr, SGN(tr->velx));
      }
      return;
   }

   /* Neither did that, hmmm.  Well, let's reverse. */
   if(_sc_traj_roller_clear(c, tr, flags, -SGN(tr->velx), 0)) {
      /* We should reverse direction now. */
      if(SC_PHYSICS_DEBUG_ROLLER) {
         printf("roller fallthru %8p  momentum reversing direction to %+d available\n",
                (void *)tr, -SGN(tr->velx));
      }
      /* TEMP - This needs attention.  If we still have roller problems, look here first.
                warning can velx sign change by calling pass_or_hit? -JDS
                warning looks like it to me (via pass_wall_elastic); is this a problem? -JTL */
      tr->velx = -tr->velx;
      return;
   }

   /* Nothing worked; return to original position */
   if(SC_PHYSICS_DEBUG_ROLLER) {
      printf("roller fallthru %8p  nothing suitable found\n", (void *)tr);
   }

}



static sc_trajectory_result _sc_traj_traverse_roller(sc_config *c, sc_trajectory *tr, int flags,
                                                     double distance, sc_trajectory_action action,
                                                     void *data) {
/* sc_traj_traverse_roller
   This function follows a roller, from source (x,y) to its (intended)
   destination <distance> units away.  If the roller is not supported
   from below, then well, it falls.  Otherwise, if the roller has a clear
   passage to the right/left then it will continue rolling merrily along
   in a horizontal fashion.  If it cannot continue rolling (wall obstruct,
   hit a tank or other ground) then it detonates.  On return, *tr will
   contain a new position (or the detonation position).  */

   sc_trajectory_result result;/* Tracking result for this function */
   sc_trajectory_data t;   /* Data for tracking (passed to subfunctions) */
   double origx;           /* Saved value for X position */
   double origy;           /* Saved value for Y position */
   double step;            /* Current step size. */

   /* Sanity checks */
   assert(c != NULL && tr != NULL);
   assert(distance >= 0);

   /* Loop until we run out of distance to travel. */
   result = SC_TRAJ_CONTINUE;
   while(distance > 0 && result == SC_TRAJ_CONTINUE) {
      /* Setup the trajectory stepping */
      step = (distance >= 1 ? 1 : distance);
      t.stepx = step * SGN(tr->velx);
      t.stepy = 0;

      /* Save the current coordinates for later, when we start
         to experiment how we can move from this current position. */
      origx = tr->curx;
      origy = tr->cury;

      if(SC_PHYSICS_DEBUG_ROLLER) {
         printf("roller traverse %8p  at %16g %16g  land is %4d  velocity is %+g\n",
                (void *)tr, origx, origy,
                sc_land_height(c->land, rint(origx), c->land->height),
                tr->velx);
         printf("roller traverse %8p  la %16g %16g\n",
                (void *)tr, tr->lastx, tr->lasty);
      }

      /* Find out if we impacted.  The type of this initial impact
         will determine whether we detonate or roll up an incline. */
      result = _sc_traj_pass_or_hit(c, tr, flags, &t);
      if(result == SC_TRAJ_CONTINUE) {
         /* This is the easy case; we were able to continue rolling
            laterally with no difficulties.  */

         /* Check for a sign reversal, e.g. from rebound on walls. */
         if(SGN(t.stepx) != SGN(tr->velx)) {
            tr->velx = -tr->velx;
         }

         /* Weapon did not hit something; continue ... */
         if(action != NULL) {
            result = action(c, tr, data);
         } /* Is user using an action function? */

         if(result == SC_TRAJ_CONTINUE) {
            /* Attempt to move; if we are not supported, then fall.
               Otherwise, roll to right/left depending on the sign
               of the velocity term. */
            /* Set lastx, lasty now that we are certain we're moving */
            tr->lastx = origx;
            tr->lasty = origy;
            _sc_traj_roller_setup(tr, &t, origx, origy, 0, -step);
            if(!SC_TRAJ_IS_IMPACT(_sc_traj_pass_or_hit(c, tr, flags, &t))) {
               /* Falling; we already modified the position, so we can
                  leave the curx, cury alone. */
               if(SC_PHYSICS_DEBUG_ROLLER) {
                  printf("roller traverse %8p  roller is currently falling at Y %+g\n",
                         (void *)tr, -step);
               }
               tr->velx += SC_TRAJ_ROLLER_FALL * SGN(tr->velx);
               _sc_traj_roller_fallthrough(c, tr, flags);
            } else {
               /* Not falling; roll in respective direction.  We need
                  to restore curx, cury.  Note that the NEXT iteration
                  will check to see if this destination was an impact. */
               if(SC_PHYSICS_DEBUG_ROLLER) {
                  printf("roller traverse %8p  roller proceeds in X %+g\n",
                         (void *)tr, SGN(tr->velx) * step);
               }
               _sc_traj_roller_restore(tr, origx + SGN(tr->velx) * step, origy);
            } /* Which direction? */
         } /* Still going? */
      } /* Did we hit something? */

      if(result == SC_TRAJ_IMPACT_LAND) {
         /* The *current* position has impacted land.  We may attempt
            to incline by a small amount here.  */
         _sc_traj_roller_setup(tr, &t, origx, origy, 0, 1);
         if(fabs(tr->velx) > SC_TRAJ_ROLLER_GAIN &&
           !SC_TRAJ_IS_IMPACT(_sc_traj_pass_or_hit(c, tr, flags, &t))) {
            /* Salvation for the little weapon! */
            /* Set lastx, lasty now that we are certain we're moving */
            tr->lastx = origx;
            tr->lasty = origy;
            if(SC_PHYSICS_DEBUG_ROLLER) {
               printf("roller traverse %8p  roller ascended to Y %+d\n",
                  (void *)tr, 1);
            }
            tr->velx -= SC_TRAJ_ROLLER_GAIN * SGN(tr->velx);
            result = SC_TRAJ_CONTINUE;
         } else {
            /* Desperation */
            _sc_traj_roller_setup(tr, &t, origx, origy, 0, 2);
            if(fabs(tr->velx) > 2 * SC_TRAJ_ROLLER_GAIN &&
              !SC_TRAJ_IS_IMPACT(_sc_traj_pass_or_hit(c, tr, flags, &t))) {
               /* We can still salvage! */
               /* Set lastx, lasty now that we are certain we're moving */
               tr->lastx = origx;
               tr->lasty = origy;
               if(SC_PHYSICS_DEBUG_ROLLER) {
                  printf("roller traverse %8p  roller ascended to Y %+d\n",
                     (void *)tr, 2);
               }
               tr->velx += -2 * SC_TRAJ_ROLLER_GAIN * SGN(tr->velx);
               result = SC_TRAJ_CONTINUE;
            } else {
               /* Cannot recover; not enough velocity. */
               if(SC_PHYSICS_DEBUG_ROLLER) {
                  printf("roller traverse %8p  roller tried to ascend, but failed.\n",
                     (void *)tr);
               }
               _sc_traj_roller_restore(tr, origx, origy);
            }
         }
      } /* Could we recover from an impact? */

      /* Next... */
      --distance;
   } /* Loop */

   /* Return our status. */
   return(result);

}



static void _sc_traj_digger_get_direction(const sc_config *c, sc_trajectory *tr,
                                          sc_trajectory_data *t, int dir) {
/* sc_traj_digger_direction_clear
   Figure out a new direction based on delta. */

   assert(c != NULL && tr != NULL && t != NULL);

   if(dir < 0) dir += 4;
   switch((tr->dir + dir) % 4) {
      case 0:  t->stepx =  0; t->stepy = -1; break;
      case 1:  t->stepx = -1; t->stepy =  0; break;
      case 2:  t->stepx =  0; t->stepy =  1; break;
      default: t->stepx =  1; t->stepy =  0; break;
   }

   tr->curx += t->stepx;
   tr->cury += t->stepy;

}



static bool _sc_traj_digger_direction_clear(const sc_config *c, sc_trajectory *tr,
                                            int flags, int dir) {
/* sc_traj_digger_direction_clear
   Check if the specified delta direction is clear or not. */

   sc_trajectory_data t;
   int result;

   assert(c != NULL && tr != NULL);

   /* Get data on our new chosen direction */
   _sc_traj_digger_get_direction(c, tr, &t, dir);
   result = _sc_traj_hit(c, tr, flags, &t);

   /* Special case for 1-pixel wide gaps */
   if(result == SC_TRAJ_CONTINUE) {
      tr->curx += t.stepx;
      tr->cury += t.stepy;
      result = _sc_traj_hit(c, tr, flags, &t);
   } /* Special case... */

   /* Check if we (eventuall) impact land next */
   if(result != SC_TRAJ_IMPACT_LAND) return(false);
   tr->dir = (tr->dir + dir) % 4;
   return(true);

}



static bool _sc_traj_digger_reselect_dir(const sc_config *c, sc_trajectory *tr, int flags) {
/* sc_traj_digger_reselect_dir
   Attempt to find a new direction to dig into.  */

   int dir;

   assert(c != NULL && tr != NULL);

   dir = game_lrand(2) * 2 - 1;
   if(_sc_traj_digger_direction_clear(c, tr, flags, dir))  return(true);
   if(_sc_traj_digger_direction_clear(c, tr, flags, -dir)) return(true);
   if(_sc_traj_digger_direction_clear(c, tr, flags, 2))    return(true);
   if(_sc_traj_digger_direction_clear(c, tr, flags, 0))    return(true);
   return(false);

}



static sc_trajectory_result _sc_traj_traverse_digger(sc_config *c, sc_trajectory *tr,
                                                     int flags, double distance,
                                                     sc_trajectory_action action, void *data) {
/* sc_traj_traverse_digger
   This function follows a digger, from source (x,y) to some destination
   after <distance> units of movement.  The digger moves around clearing
   land and randomly changing direction.  If it leaves the dirt for any
   reason, it fizzles.  */

/* TEMP:  Digger code is probably broken.  It was never really completed. */

   const int *gradient;    /* Sky gradient */
   int gradientflag;       /* Sky gradient flag */
   bool dither;            /* Enable dithering? */
   sc_trajectory_result result;/* Tracking result for this function */
   sc_trajectory_data t;   /* Data for tracking (passed to subfunctions) */
   double step;            /* Current step. */

   assert(c != NULL && tr != NULL);

   /* Get the sky gradient */
   gradient = sc_land_sky_index(c);
   gradientflag = sc_land_sky_flag(c);
   dither = c->graphics.gfxdither;

   result = SC_TRAJ_CONTINUE;
   while(distance > 0 && result == SC_TRAJ_CONTINUE) {
      /* Setup the trajectory stepping */
      step = (distance >= 1 ? 1 : distance);

      /* Check if we hit anything */
      _sc_traj_digger_get_direction(c, tr, &t, 0);
      result = _sc_traj_hit(c, tr, flags, &t);
      switch(result) {
      case SC_TRAJ_IMPACT_LAND:
         /* We're okay. */
         result = SC_TRAJ_CONTINUE;
         /* Call the tracking function. */
         if(action != NULL) {
            result = action(c, tr, data);
         } /* Is user using an action function? */
         if(result == SC_TRAJ_CONTINUE) {
            if(game_lrand(100) < SC_TRAJ_DIGGER_PROB) {
               /* Randomly change directions */
               _sc_traj_digger_reselect_dir(c, tr, flags);
            }
         }
         break;

      case SC_TRAJ_IMPACT_TANK:
      case SC_TRAJ_IMPACT_SHIELD:
         /* Death to the tank! */
         break;

      case SC_TRAJ_IMPACT_WALL:
      case SC_TRAJ_IMPACT_GROUND:
      case SC_TRAJ_CONTINUE:
      case SC_TRAJ_SIZZLE:
         /* We're dead. */
         result = SC_TRAJ_SIZZLE;
         break;
      }

      /* Next... */
      --distance;
   } /* Loop */

   /* Return our status. */
   return(result);

}



static sc_trajectory_result _sc_traj_step_flight(sc_config *c, sc_trajectory *tr, int flags,
                                                 sc_trajectory_action action, void *data) {
/* sc_traj_step_flight */

   sc_trajectory_result result;  /* Result of this function */
   double locx, locy;            /* Location buffers */
   double velx, vely;            /* Velocity buffers */

   assert(c != NULL && tr != NULL);

   while(tr->timestep <= tr->finalstep) {
      tr->oldx = tr->curx;
      tr->oldy = tr->cury;
      tr->curx = sc_traj_get_current_x(tr);
      tr->cury = sc_traj_get_current_y(tr);
      result = _sc_traj_traverse_path(c, tr, flags, action, data);
      /* This test turns a shield impact into a rebound for force shields */
      if(result == SC_TRAJ_IMPACT_SHIELD && !(flags & SC_TRAJ_HIT_SHIELDS)) {
         /* We want the real location of the weapon, not the interpolated one here. */
         locx = tr->curx;
         locy = tr->cury;
         /* The real velocities of the weapon would be nice but aren't known... */
         velx = sc_traj_get_velocity_x(tr);
         vely = sc_traj_get_velocity_y(tr);
         /* Ask the shield if it will be rebounding this missile. */
         if(sc_shield_get_reflection(c, c->players[tr->victim], flags,
                                     &locx, &locy, &velx, &vely)) {
            sc_traj_new_control_equation(tr, locx, locy, velx, vely);
            result = SC_TRAJ_CONTINUE;
            tr->curx = locx;
            tr->cury = locy;
            tr->victim = -1;
         }
      }
      if(result != SC_TRAJ_CONTINUE) return(result);
      if(tr->timestep < tr->finalstep) {
         tr->timestep = min(tr->timestep + SC_TRAJ_TIME_STEPS_PER_SUBSTEP, tr->finalstep);
      } else {
         tr->timestep += SC_TRAJ_TIME_STEPS_PER_SUBSTEP;
      }

      /* Perform actions normally done at the interpolation vertices here */
      if(!(flags & SC_TRAJ_IGNORE_TANK)) {
         /* Perform magnetic shield deflection. */
         velx = sc_traj_get_velocity_x(tr);
         vely = sc_traj_get_velocity_y(tr);
         if(sc_shield_get_deflection(c, tr->player, flags,
                                     sc_traj_get_current_x(tr),
                                     sc_traj_get_current_y(tr),
                                     &velx, &vely))
            sc_traj_revise_velocity(tr, velx, vely);
      }
   }
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_traj_step_roller(sc_config *c, sc_trajectory *tr, int flags,
                                                 sc_trajectory_action action, void *data) {
/* sc_traj_step_roller */

   sc_trajectory_result result;  /* Result of this function */
   double distance;              /* Distance travelling. */

   assert(c != NULL && tr != NULL);

   while(tr->timestep <= tr->finalstep) {
      tr->oldx = tr->curx;
      tr->oldy = tr->cury;
      distance = fabs(tr->velx);
      result = _sc_traj_traverse_roller(c, tr, flags, distance, action, data);
      if(result != SC_TRAJ_CONTINUE) return(result);
      if(tr->timestep < tr->finalstep) {
         tr->timestep = min(tr->timestep + SC_TRAJ_TIME_STEPS_PER_SUBSTEP, tr->finalstep);
      } else {
         tr->timestep += SC_TRAJ_TIME_STEPS_PER_SUBSTEP;
      }
   }
   return(SC_TRAJ_CONTINUE);

}



static sc_trajectory_result _sc_traj_step_digger(sc_config *c, sc_trajectory *tr, int flags,
                                                 sc_trajectory_action action, void *data) {
/* sc_traj_step_digger */

   sc_trajectory_result result;  /* Result of this function */
   double distance;              /* Distance travelling. */

   assert(c != NULL && tr != NULL);

   while(tr->timestep <= tr->finalstep) {
      tr->oldx = tr->curx;
      tr->oldy = tr->cury;
      distance = fabs(tr->timestep * tr->velx);
      result = _sc_traj_traverse_digger(c, tr, flags, distance, action, data);
      if(result != SC_TRAJ_CONTINUE) return(result);
      if(tr->timestep < tr->finalstep) {
         tr->timestep = min(tr->timestep + SC_TRAJ_TIME_STEPS_PER_SUBSTEP, tr->finalstep);
      } else {
         tr->timestep += SC_TRAJ_TIME_STEPS_PER_SUBSTEP;
      }
   }
   return(SC_TRAJ_CONTINUE);

}



sc_trajectory_result sc_traj_step(sc_config *c, sc_trajectory *tr, int flags,
                                  sc_trajectory_action action, void *data) {
/* sc_traj_step
   Run one step of a trajectory.  Used for code that intends to
   actually animate the trajectory.  Only one timestep is run.  */

   sc_trajectory_result result;  /* Result of this function */

   assert(c != NULL);

   if(tr == NULL) {
      return(SC_TRAJ_SIZZLE);
   }

   if(tr->timestep >= tr->stopstep) {
      /* Weapon timed out */
      return(SC_TRAJ_SIZZLE);
   }

   tr->finalstep = min(tr->timestep + SC_TRAJ_TIME_STEP, tr->stopstep);

   result = SC_TRAJ_SIZZLE;
   switch(tr->type) {
      case SC_TRAJ_FLIGHT:
         result = _sc_traj_step_flight(c, tr, flags, action, data);
         break;
      case SC_TRAJ_ROLLER:
         result = _sc_traj_step_roller(c, tr, flags, action, data);
         break;
      case SC_TRAJ_DIGGER:
         result = _sc_traj_step_digger(c, tr, flags, action, data);
         break;
   }

   if(result != SC_TRAJ_CONTINUE && !(flags & SC_TRAJ_NO_MODIFY)) {
      sc_ai_trajectory_terminus(c, tr);
   }

   return(result);

}



sc_trajectory_result sc_traj_run(sc_config *c, sc_trajectory *tr, int flags,
                                 sc_trajectory_action action, void *data) {
/* sc_traj_run
   Runs an entire trajectory to completion.  Useful for simulations.  */

   sc_trajectory_result result;  /* Result of this function */

   do {
      result = sc_traj_step(c, tr, flags, action, data);
   } while(result == SC_TRAJ_CONTINUE);
   return(result);

}



void sc_traj_revise_velocity(sc_trajectory *tr, double nvelx, double nvely) {
/* sc_traj_revise_velocity
   Call this function when you want to revise the course of the weapon
   already in flight.  Assumptions:  we want to maintain current position
   but revise the current velocity.  We do NOT want to revise the time
   step value for this trajectory.  We MAY revise the initial velocity
   and initial position.

   Let cx = current X, a = acceleration (const), t = timestep (const),
   v1 represent revised init velocity, x1 represent revised init pos.
   Then to attain a new velocity nv, we want

      cx = x1 + v1 t + a t^2 / 2
      nv = v1 + a t

   This system gives us the solution

      v1 = nv - a t
      x1 = cx - nv t + a t^2 / 2

   This function should be called instead of set_velocity once the weapon
   has been launched (and has a nonzero timestep).  DO NOT MODIFY THE
   VELOCITY DIRECTLY or you will get very inconsistent tracking results.
   Be careful when doing this inside a linear interpolation (a call to
   sc_traj_traverse_path()) -- make sure stepx, stepy for the interpolation
   are somehow revised as well so we do not stray too far from the path.  */

   double curx;
   double cury;

   /* Sanity checks */
   assert(tr != NULL);
   if(tr->type != SC_TRAJ_FLIGHT) {
      fprintf(stderr, "WARNING:  does not make sense to set velocity of non-airborn trajectories!\n");
      return;
   }

   /* Rewrite initial equation parametres */
   curx = sc_traj_get_current_x(tr);   /* These functions depend */
   cury = sc_traj_get_current_y(tr);   /* on the old parametres */
   tr->velx = nvelx - tr->timestep * tr->accx;
   tr->vely = nvely + tr->timestep * tr->accy;
   tr->ctrx = curx - tr->timestep * (tr->velx + 0.5 * tr->timestep * tr->accx);
   tr->ctry = cury - tr->timestep * (tr->vely - 0.5 * tr->timestep * tr->accy);

}



void sc_traj_set_velocity(sc_trajectory *tr, double velx, double vely) {
/* sc_traj_set_velocity */

   sc_traj_revise_velocity(tr, velx, vely);

}



void sc_traj_revise_acceleration(sc_trajectory *tr, double naccx, double naccy) {
/* sc_traj_revise_acceleration
   Call this function when you want to revise the course of the weapon
   already in flight via its acceleration terms.  Assumptions:  we want
   to maintain current position but revise the constant acceleration
   terms.  We do NOT want to revise the time step value for this trajectory.
   We MAY revise the initial velocity and initial position.

   Let cx = current X, na = new acceleration term, t = timestep (const),
   and let cv = current velocity.  Then to attain a new const acceleration
   term na, we want

      cx = x1 + v1 t + na t^2 / 2
      cv = v1 + na t

   This system gives us the solution

      v1 = cv - na t
      x1 = cx - v1 t - na t^2 / 2

   This function should be called instead of modifying acceleration terms
   directly, for FLIGHT weapons.  */

   double curx;
   double cury;
   double velx;
   double vely;

   /* Sanity checks */
   assert(tr != NULL);
   if(tr->type != SC_TRAJ_FLIGHT) {
      fprintf(stderr, "WARNING:  does not make sense to set acceleration of non-airborn trajectories!\n");
      return;
   }

   /* Rewrite initial equation parametres */
   curx = sc_traj_get_current_x(tr);   /* These functions depend */
   cury = sc_traj_get_current_y(tr);   /* on the old parametres */
   velx = sc_traj_get_velocity_x(tr);
   vely = sc_traj_get_velocity_y(tr);
   tr->velx = velx - tr->timestep * naccx;
   tr->vely = vely + tr->timestep * naccy;
   tr->ctrx = curx - tr->timestep * (tr->velx + 0.5 * tr->timestep * naccx);
   tr->ctry = cury - tr->timestep * (tr->vely - 0.5 * tr->timestep * naccy);
   tr->accx = naccx;
   tr->accy = naccy;
   /* This does not revise original accelerations */

}



void sc_traj_restore_acceleration(sc_trajectory *tr) {
/* sc_traj_restore_acceleration
   Restores acceleration to the parametres it was at initially
   (when the weapon was first launched).  This is useful, FE,
   if you want to apply short-term bursts of thrust, or want
   to travel through a mountain :) */

   sc_traj_revise_acceleration(tr, tr->oaccx, tr->oaccy);

}



void sc_traj_new_control_equation(sc_trajectory *tr,
                                  double ctrx, double ctry,
                                  double velx, double vely) {
/* sc_traj_new_control_equation
   Writes a new control equation, while PRESERVING timestep
   it will recenter so at the current timestep, the weapon
   would be at the position and with the velocity indicated.
   ONLY use this function at an interpolation vertex !!  The
   current REAL position and desired new velocity are passed
   in.  Current acceleration and timestep will be preserved.
   This algorithm models the revise_acceleration algorithm.  */

   /* Sanity checks */
   assert(tr != NULL);
   if(tr->type != SC_TRAJ_FLIGHT) {
      fprintf(stderr, "WARNING:  does not make sense to set acceleration of non-airborn trajectories!\n");
      return;
   }

   /* Rewrite initial equation parametres */
   tr->velx = velx - tr->timestep * tr->accx;
   tr->vely = vely + tr->timestep * tr->accy;
   tr->ctrx = ctrx - tr->timestep * (tr->velx + 0.5 * tr->timestep * tr->accx);
   tr->ctry = ctry - tr->timestep * (tr->vely - 0.5 * tr->timestep * tr->accy);

}



double sc_traj_get_velocity_x(const sc_trajectory *tr) {
/* sc_traj_get_velocity_x
   dx/dt  =  velx + accx t  */

   if(tr == NULL) return(0);
   return(tr->velx + tr->accx * tr->timestep);

}



double sc_traj_get_velocity_y(const sc_trajectory *tr) {
/* sc_traj_get_velocity_y
   dy/dt  =  vely - accy t  */

   if(tr == NULL) return(0);
   return(tr->vely - tr->accy * tr->timestep);

}



double sc_traj_get_current_x(const sc_trajectory *tr) {
/* sc_traj_get_current_x
   x  =  ctrx + velx t + 0.5 accx t^2  */

   if(tr == NULL) return(0);
   return(tr->ctrx + tr->timestep * (tr->velx + 0.5 * tr->timestep * tr->accx));

}



double sc_traj_get_current_y(const sc_trajectory *tr) {
/* sc_traj_get_current_y
   y  =  ctry + vely t - 0.5 accy t^2  */

   if(tr == NULL) return(0);
   return(tr->ctry + tr->timestep * (tr->vely - 0.5 * tr->timestep * tr->accy));

}



double sc_traj_height(const sc_trajectory *tr) {
/* sc_ai_trajectory_height
   Calculates height attained by this trajectory.
      dy/dt = 0 = vy - G t
         vy = G t  -->  t = vy/G
          y = vy t - 1/2 G t^2 = vy^2 / (2 G)     */

   double vy2;       /* Velocity of Y, squared. */

   /* Calculate velocity squared, and factor in the current gravity. */
   vy2 = SQR(tr->vely);
   return(0.5 * vy2 / tr->accy);

}



/***     Parametre Names      ***/



static const char *_sc_physics_wall_names[] = {
   "None",
   "Concrete",
   "Padded",
   "Rubber",
   "Springy",
   "Wraparound",
   "Random",
   NULL
};
static const unsigned int _sc_physics_wall_types[] = {
   SC_WALL_NONE,
   SC_WALL_CONCRETE,
   SC_WALL_PADDED,
   SC_WALL_RUBBER,
   SC_WALL_SPRING,
   SC_WALL_WRAP,
   SC_WALL_RANDOM,
   0
};



const char **sc_physics_wall_names(void) {

   return(_sc_physics_wall_names);

}



const unsigned int *sc_physics_wall_types(void) {

   return(_sc_physics_wall_types);

}
