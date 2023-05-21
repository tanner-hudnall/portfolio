package paxos;

import java.io.Serializable;

/**
 * Please fill in the data structure you use to represent the request message
 * for each RMI call.
 * Hint: You may need the sequence number for each paxos instance and also you
 * may need proposal number and value.
 * Hint: Make it more generic such that you can use it for each RMI call.
 * Hint: It is easier to make each variable public
 */
public class Request implements Serializable { // Anthony
    static final long serialVersionUID = 1L;

    public int sequenceNumber;
    public int proposalNumber;
    public int senderID;
    public int promisedNumber;

    public Object value;
    public int highestDoneNumber;
    public boolean promiseRequest;

    public Request(int promisedNumber, int senderID) { // promise request
        this.senderID = senderID;
        this.promisedNumber = promisedNumber;
        this.promiseRequest = true;
    }

    public Request(int sequenceNumber, int proposalNumber, int senderID, Object value, int highestDoneNumber) { // normal
                                                                                                                // request
        this.sequenceNumber = sequenceNumber;
        this.proposalNumber = proposalNumber;
        this.senderID = senderID;
        this.value = value;
        this.promiseRequest = false;
        this.highestDoneNumber = highestDoneNumber;
    }

}