package us.ihmc.aci.kernel;

/**
 * Exception that means an invalid request has been issued to a service instance
 * and the state of the latter did not allow it
 *
 * @author rquitadamo
 */
class IllegalStateTransition extends Exception {

    IllegalStateTransition(String message) {
        super(message);
    }

}
