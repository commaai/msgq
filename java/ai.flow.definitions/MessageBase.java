package ai.flow.definitions;

import org.capnproto.*;

import java.io.IOException;
import java.nio.ByteBuffer;
import static ai.flow.common.utils.secSinceBoot;

public abstract class MessageBase {

    public enum AllocationStyle {
        HEAP,
        OFF_HEAP;
    }
    public static final DefaultAllocator.ByteBufferAllocationStyle style = DefaultAllocator.ByteBufferAllocationStyle.DIRECT;
    public final Allocator allocator = new DefaultAllocator(style);
    public static final int BYTES_PER_WORD = 8;
    public Definitions.Event.Builder event;
    public MessageBuilder messageBuilder;
    public int bytesRawMessage;
    public int bytesSerializedForm;
    public ByteBuffer serializedBuffer;
    public ByteBuffer rawMessageBuffer;
    private ArrayOutputStream stream;

    public MessageBase(ByteBuffer rawMessageBuffer){
        this.rawMessageBuffer = rawMessageBuffer;
        messageBuilder = new MessageBuilder(rawMessageBuffer);
    }

    public MessageBase(){
        messageBuilder = new MessageBuilder(allocator);
    }

    public int computeAllocatedBytes(){
        int allocated = 0;
        for (ByteBuffer buffer : messageBuilder.getSegmentsForOutput()) {
            allocated += buffer.limit();
        }
        return allocated;
    }

    public int  computeSerializedMsgBytes(){
        return BYTES_PER_WORD * (int) Serialize.computeSerializedSizeInWords(messageBuilder);
    }

    public void initSerializedBuffer() {
        serializedBuffer = ByteBuffer.allocateDirect(bytesSerializedForm);
        stream = new ArrayOutputStream(serializedBuffer);
    }

    public ByteBuffer getSerializedBuffer(){
        if (serializedBuffer == null)
            initSerializedBuffer();
        return serializedBuffer;
    }

    public ByteBuffer serialize(boolean valid) {
        try {
            event.setLogMonoTime((long)(secSinceBoot() * 1e9));
            event.setValid(valid);
            stream.buf.rewind();
            Serialize.write(stream, messageBuilder);
        } catch (IOException e) {
            System.out.println(e);
        }
        return serializedBuffer;
    }

    /** Used to load message bytes obtained from `to_bytes() method`*/
    public static Definitions.Event.Reader deserialize(ByteBuffer buffer) {
        try {
            MessageReader messageReader = Serialize.read(buffer);
            buffer.rewind();
            return messageReader.getRoot(Definitions.Event.factory);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    public Definitions.Event.Reader deserialize() {
        try {
            MessageReader messageReader = Serialize.read(serializedBuffer);
            serializedBuffer.rewind();
            return messageReader.getRoot(Definitions.Event.factory);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }
}
