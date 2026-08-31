```c
/*
 * Differential-drive recruitment task
 */

#include "drive.h"

#define PI_F 3.14159265358979323846f

#define WHEEL_RADIUS 0.15f
#define WHEEL_SEPARATION 0.77f
#define MAX_LINEAR_VELOCITY 1.0f
#define MAX_ANGULAR_VELOCITY 2.0f
#define MAX_WHEEL_VELOCITY 10.0f
#define HEADING_GAIN 1.25f

#define TARGET_TOLERANCE 0.10f
#define DRIVE_DT_SECONDS 0.02f
#define MAX_DRIVE_STEPS 6000


static float clampf(float value, float min, float max)
{
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}


static float normalize_angle(float angle)
{
    while (angle > PI_F) {
        angle -= 2.0f * PI_F;
    }

    while (angle < -PI_F) {
        angle += 2.0f * PI_F;
    }

    return angle;
}


static bool coordinate_is_finite(const struct coordinate *coordinate)
{
    if (coordinate == NULL) {
        return false;
    }

    return isfinite(coordinate->latitude) &&
           isfinite(coordinate->longitude) &&
           isfinite(coordinate->altitude);
}


static bool rover_is_valid(const struct rover_state *rover)
{
    if (rover == NULL) {
        return false;
    }

    return coordinate_is_finite(&rover->position) &&
           isfinite(rover->heading_rad);
}


static struct wheel_velocity
limit_wheel_velocities(struct wheel_velocity velocity)
{
    velocity.left =
        clampf(velocity.left,
               -MAX_WHEEL_VELOCITY,
               MAX_WHEEL_VELOCITY);

    velocity.right =
        clampf(velocity.right,
               -MAX_WHEEL_VELOCITY,
               MAX_WHEEL_VELOCITY);

    return velocity;
}


/* Provided simulator helper */
static bool apply_wheel_velocities(
    struct rover_state *rover,
    struct wheel_velocity velocity)
{
    if (!isfinite(velocity.left) ||
        !isfinite(velocity.right) ||
        fabsf(velocity.left) > MAX_WHEEL_VELOCITY ||
        fabsf(velocity.right) > MAX_WHEEL_VELOCITY) {

        return false;
    }

    const float linear_velocity =
        WHEEL_RADIUS *
        (velocity.left + velocity.right) / 2.0f;

    const float angular_velocity =
        WHEEL_RADIUS *
        (velocity.right - velocity.left) /
        WHEEL_SEPARATION;

    rover->heading_rad = normalize_angle(
        rover->heading_rad +
        angular_velocity * DRIVE_DT_SECONDS);

    rover->position.longitude +=
        linear_velocity *
        cosf(rover->heading_rad) *
        DRIVE_DT_SECONDS;

    rover->position.latitude +=
        linear_velocity *
        sinf(rover->heading_rad) *
        DRIVE_DT_SECONDS;

    return true;
}


/*
 * Drive the rover towards the target.
 */
enum drive_status drive_to_target(
    struct rover_state *rover,
    const struct coordinate *target)
{
    if (!rover_is_valid(rover) ||
        !coordinate_is_finite(target)) {

        return DRIVE_INVALID_INPUT;
    }

    rover->heading_rad =
        normalize_angle(rover->heading_rad);

    for (int step = 0;
         step < MAX_DRIVE_STEPS;
         step++) {

        /*
         * Calculate the difference between the rover
         * and the target.
         */
        float dx =
            target->longitude -
            rover->position.longitude;

        float dy =
            target->latitude -
            rover->position.latitude;

        float distance = hypotf(dx, dy);

        /*
         * Stop when the rover reaches the target.
         */
        if (distance <= TARGET_TOLERANCE) {
            return DRIVE_REACHED_TARGET;
        }

        /*
         * Calculate the desired direction.
         *
         * atan2(y, x):
         * y = latitude
         * x = longitude
         */
        float desired_heading = atan2f(dy, dx);

        /*
         * Calculate heading error and normalize it
         * to handle wraparound.
         */
        float heading_error =
            normalize_angle(
                desired_heading -
                rover->heading_rad);

        /*
         * Angular velocity proportional to
         * heading error.
         */
        float angular_velocity =
            HEADING_GAIN * heading_error;

        angular_velocity =
            clampf(
                angular_velocity,
                -MAX_ANGULAR_VELOCITY,
                MAX_ANGULAR_VELOCITY);

        /*
         * Move forward more slowly when the rover
         * is facing away from the target.
         */
        float linear_velocity =
            MAX_LINEAR_VELOCITY * cosf(heading_error);

        if (linear_velocity < 0.0f) {
            linear_velocity = 0.0f;
        }

        linear_velocity =
            clampf(
                linear_velocity,
                0.0f,
                MAX_LINEAR_VELOCITY);

        /*
         * Convert linear and angular velocity
         * to left and right wheel velocities.
         */
        struct wheel_velocity velocity;

        velocity.left =
            (linear_velocity -
             angular_velocity *
             WHEEL_SEPARATION / 2.0f)
            / WHEEL_RADIUS;

        velocity.right =
            (linear_velocity +
             angular_velocity *
             WHEEL_SEPARATION / 2.0f)
            / WHEEL_RADIUS;

        /*
         * Ensure wheel limits are respected.
         */
        velocity =
            limit_wheel_velocities(velocity);

        /*
         * Apply the movement to the simulator.
         */
        if (!apply_wheel_velocities(
                rover,
                velocity)) {

            return DRIVE_INVALID_COMMAND;
        }
    }

    return DRIVE_MAX_STEPS_EXCEEDED;
}
```
