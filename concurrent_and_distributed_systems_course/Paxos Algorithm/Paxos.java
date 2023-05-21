package paxos;

import java.rmi.registry.LocateRegistry;
import java.rmi.server.UnicastRemoteObject;
import java.rmi.registry.Registry;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.ReentrantLock;
import java.util.*;
import java.util.concurrent.*;

/**
 * This class is the main class you need to implement paxos instances.
 * It corresponds to a single Paxos peer.
 */
public class Paxos implements PaxosRMI, Runnable {
    ReentrantLock mutex;
    String[] peers; // hostnames of all peers
    int[] ports; // ports of all peers
    int me; // this peer's index into peers[] and ports[]

    Registry registry;
    PaxosRMI stub;

    AtomicBoolean dead; // for testing
    AtomicBoolean unreliable; // for testing

    // Your data here
    ConcurrentHashMap<Integer, PaxosInstance> paxos_instances;
    int sequence;
    int[] finish_tracker;
    Semaphore Lock;

    public class PaxosInstance {
        State state;
        int proposalNum;
        int acceptorId;
        Object value;

        public PaxosInstance(int proposalNum, int acceptorId, Object value) {
            this.state = State.Pending;
            this.proposalNum = proposalNum;
            this.acceptorId = acceptorId;
            this.value = value;
        }
    }

    /**
     * Call the constructor to create a Paxos peer.
     * The hostnames of all the Paxos peers (including this one)
     * are in peers[]. The ports are in ports[].
     */
    public Paxos(int me, String[] peers, int[] ports) {
        this.me = me;
        this.peers = peers;
        this.ports = ports;
        this.mutex = new ReentrantLock();
        this.dead = new AtomicBoolean(false);
        this.unreliable = new AtomicBoolean(false);

        // Your initialization code here
        this.sequence = 0;
        this.finish_tracker = new int[peers.length];
        for (int i = 0; i < this.finish_tracker.length; i++) {
            this.finish_tracker[i] = -1;
        }

        this.paxos_instances = new ConcurrentHashMap<Integer, PaxosInstance>();
        Lock = new Semaphore(1);

        // register peers, do not modify this part
        try {
            System.setProperty("java.rmi.server.hostname", this.peers[this.me]);
            registry = LocateRegistry.createRegistry(this.ports[this.me]);
            stub = (PaxosRMI) UnicastRemoteObject.exportObject(this, this.ports[this.me]);
            registry.rebind("Paxos", stub);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Call() sends an RMI to the RMI handler on server with
     * arguments rmi name, request message, and server id. It
     * waits for the reply and return a response message if
     * the server responded, and return null if Call() was not
     * be able to contact the server.
     *
     * You should assume that Call() will time out and return
     * null after a while if it doesn't get a reply from the server.
     *
     * Please use Call() to send all RMIs and please don't change
     * this function.
     */
    public Response Call(String rmi, Request req, int id) {
        Response callReply = null;

        PaxosRMI stub;
        try {
            Registry registry = LocateRegistry.getRegistry(this.ports[id]);
            stub = (PaxosRMI) registry.lookup("Paxos");
            if (rmi.equals("Prepare"))
                callReply = stub.Prepare(req);
            else if (rmi.equals("Accept"))
                callReply = stub.Accept(req);
            else if (rmi.equals("Decide"))
                callReply = stub.Decide(req);
            else
                System.out.println("Wrong parameters!");
        } catch (Exception e) {
            return null;
        }
        return callReply;
    }

    /**
     * The application wants Paxos to start agreement on instance seq,
     * with proposed value v. Start() should start a new thread to run
     * Paxos on instance seq. Multiple paxos_instances can be run concurrently.
     *
     * Hint: You may start a thread using the runnable interface of
     * Paxos object. One Paxos object may have multiple paxos_instances, each
     * instance corresponds to one proposed value/command. Java does not
     * support passing arguments to a thread, so you may reset seq and v
     * in Paxos object before starting a new thread. There is one issue
     * that variable may change before the new thread actually reads it.
     * Test won't fail in this case.
     *
     * Start() just starts a new thread to initialize the agreement.
     * The application will call Status() to find out if/when agreement
     * is reached.
     */
    public void Start(int seq, Object value) { // Tanner
        if (seq >= Min()) {
            paxos_instances.put(seq, new PaxosInstance(0, 0, value));

            try {
                Lock.acquire();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            sequence = seq;
            Thread t = new Thread(this);
            t.start();
        }
    }

    // TODO: Tanner can you check this for logic
    // T: it's good
    @Override
    public void run() {
        int seq = sequence;
        Lock.release();
        PaxosInstance pI = paxos_instances.get(seq);

        while (!isDead() && (pI.state != State.Decided)) {
            pI.proposalNum++;
            Request prepReq = new Request(seq, pI.proposalNum, me, null, finish_tracker[me]);

            int prepareApp = 0;
            int maxNA = pI.acceptorId;
            Object maxVA = pI.value;

            for (int i = 0; i < peers.length; i++) {
                Response res = (i == me) ? Prepare(prepReq) : Call("Prepare", prepReq, i);

                if (res != null && res.isAccepted) {
                    prepareApp++;
                    if (res.acceptorID > maxNA) {
                        maxNA = res.acceptorID;
                        maxVA = res.value;
                    }
                } else if (res != null && !res.isAccepted) {
                    pI.proposalNum = Math.max(pI.proposalNum, res.proposalNumber);
                }
            }

            if (prepareApp > peers.length / 2) {
                int acceptApp = 0;
                Request accReq = new Request(seq, pI.proposalNum, me, maxVA, finish_tracker[me]);

                for (int i = 0; i < peers.length; i++) {
                    Response res = (i == me) ? Accept(accReq) : Call("Accept", accReq, i);

                    if (res != null && res.isAccepted) {
                        acceptApp++;
                    }
                }

                if (acceptApp > peers.length / 2) {
                    Request decReq = new Request(seq, 0, me, maxVA, finish_tracker[me]);

                    for (int i = 0; i < peers.length; i++) {
                        Response res = (i == me) ? Decide(decReq) : Call("Decide", decReq, i);
                    }
                }
            }
        }
    }

    // TODO: Tanner
    // RMI Handler for prepare requests
    public Response Prepare(Request r) {
        finish_tracker[r.senderID] = r.highestDoneNumber;
        clearPaxosInstances();

        PaxosInstance pI = paxos_instances.computeIfAbsent(r.sequenceNumber,
                seqNum -> new PaxosInstance(-1, -1, null));
        boolean isAccepted = false;

        if (r.proposalNumber > pI.proposalNum || me == r.senderID) {
            pI.proposalNum = r.proposalNumber;
            isAccepted = true;
        }

        return new Response(r.sequenceNumber, pI.proposalNum, pI.acceptorId, pI.value, isAccepted);
    }

    // TODO: Tanner
    // RMI Handler for accept requests
    public Response Accept(Request r) {
        finish_tracker[r.senderID] = r.highestDoneNumber;
        clearPaxosInstances();

        PaxosInstance pI = paxos_instances.get(r.sequenceNumber);
        boolean isAccepted = false;

        if (r.proposalNumber >= pI.proposalNum) {
            pI.proposalNum = r.proposalNumber;
            pI.acceptorId = r.proposalNumber;
            pI.value = r.value;
            isAccepted = true;
        }

        return new Response(r.sequenceNumber, r.proposalNumber, 0, null, isAccepted);
    }

    // TODO: Tanner
    // RMI Handler for decide requests
    public Response Decide(Request r) {
        finish_tracker[r.senderID] = r.highestDoneNumber;
        clearPaxosInstances();

        boolean isAccepted = false;

        if (!r.promiseRequest) {
            PaxosInstance pI = paxos_instances.get(r.sequenceNumber);
            pI.state = State.Decided;
            pI.value = r.value;
            isAccepted = true;
        }

        return new Response(r.sequenceNumber, 0, 0, null, isAccepted);
    }

    /**
     * The application on this machine is finish_tracker with
     * all paxos_instances <= seq.
     *
     * see the comments for Min() for more explanation.
     */

    // TODO: Tanner
    public void Done(int seq) {
        finish_tracker[me] = seq;
    }

    /**
     * The application wants to know the
     * highest instance sequence known to
     * this peer.
     */
    public int Max() { // Anthony
        return paxos_instances.keySet().stream().max(Integer::compare).orElse(Integer.MIN_VALUE);
    }

    /**
     * Min() should return one more than the minimum among z_i,
     * where z_i is the highest number ever passed
     * to Done() on peer i. A peers z_i is -1 if it has
     * never called Done().
     * Paxos is required to have forgotten all information
     * about any paxos_instances it knows that are < Min().
     * The point is to free up memory in long-running
     * Paxos-based servers.
     * Paxos peers need to exchange their highest Done()
     * arguments in order to implement Min(). These
     * exchanges can be piggybacked on ordinary Paxos
     * agreement protocol messages, so it is OK if one
     * peers Min does not reflect another Peers Done()
     * until after the next instance is agreed to.
     * The fact that Min() is defined as a minimum over
     * all Paxos peers means that Min() cannot increase until
     * all peers have been heard from. So if a peer is dead
     * or unreachable, other peers Min()s will not increase
     * even if all reachable peers call Done. The reason for
     * this is that when the unreachable peer comes back to
     * life, it will need to catch up on paxos_instances that it
     * missed -- the other peers therefore cannot forget these
     * paxos_instances.
     */
    public int Min() { // Anthony
        int min = Arrays.stream(finish_tracker)
                .map(num -> num + 1)
                .min()
                .orElse(Integer.MAX_VALUE);
        return min;
    }

    public void clearPaxosInstances() { // Anthony
        int min = Min();
        paxos_instances.keySet().removeIf(key -> key < min);
    }

    /**
     * The application wants to know whether this
     * peer thinks an instance has been decided,
     * and if so what the agreed value is. Status()
     * should just inspect the local peer state;
     * it should not contact other Paxos peers.
     */
    public retStatus Status(int seq) { // Tanner
        PaxosInstance pI = paxos_instances.get(seq);
        if (pI == null) {
            return new retStatus(State.Pending, null);
        } else if (seq < Min()) {
            return new retStatus(State.Forgotten, null);
        } else {
            return new retStatus(pI.state, pI.value);
        }
    }

    /**
     * helper class for Status() return
     */
    public class retStatus {
        public State state;
        public Object v;

        public retStatus(State state, Object v) {
            this.state = state;
            this.v = v;
        }
    }

    /**
     * Tell the peer to shut itself down.
     * For testing.
     * Please don't change these four functions.
     */
    public void Kill() {
        this.dead.getAndSet(true);
        if (this.registry != null) {
            try {
                UnicastRemoteObject.unexportObject(this.registry, true);
            } catch (Exception e) {
                System.out.println("None reference");
            }
        }
    }

    public boolean isDead() {
        return this.dead.get();
    }

    public void setUnreliable() {
        this.unreliable.getAndSet(true);
    }

    public boolean isunreliable() {
        return this.unreliable.get();
    }
}
