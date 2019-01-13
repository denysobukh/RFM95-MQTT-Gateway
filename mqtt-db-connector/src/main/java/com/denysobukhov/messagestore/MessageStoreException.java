package com.denysobukhov.messagestore;

public class MessageStoreException extends Exception {
    public MessageStoreException() {
        super();
    }

    public MessageStoreException(String m) {
        super(m);
    }

    MessageStoreException(Exception e) {
        super(e);
    }
}
