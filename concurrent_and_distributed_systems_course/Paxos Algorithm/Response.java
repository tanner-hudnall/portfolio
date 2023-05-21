package paxos;

import java.io.Serializable;

/**
 * Please fill in the data structure you use to represent the response message
 * for each RMI call.
 * Hint: You may need a boolean variable to indicate ack of acceptors and also
 * you may need proposal number and value.
 * Hint: Make it more generic such that you can use it for each RMI call.
 */
public class Response implements Serializable { // Anthony
    static final long serialVersionUID = 2L;
    // Your data here
    public int sequenceNumber;
    int proposalNumber;
    public Object value;
    public boolean isAccepted;
    int acceptorID;

    // Your constructor and methods here
    public Response(int sequenceNumber, int proposalNumber, int acceptorID, Object value, boolean isAccepted) {
        this.sequenceNumber = sequenceNumber;
        this.proposalNumber = proposalNumber;
        this.value = value;

        this.acceptorID = acceptorID;
        this.isAccepted = isAccepted;

    }
}
