package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;

public class AutoVacuumOverReadTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {

        database.execSQL("BEGIN EXCLUSIVE");

        String createTableMessages = "create table Message (_id integer primary key autoincrement, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);";
        String createTableMessagesUpdates = "create table Message_Updates (_id integer unique, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);";
        String createTableMessageDeletes = "create table Message_Deletes (_id integer unique, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);";

        database.execSQL(createTableMessageDeletes);
        database.execSQL(createTableMessagesUpdates);
        database.execSQL(createTableMessages);

        database.execSQL("COMMIT TRANSACTION");

        return true;
    }

    @Override
    public String getName() {
        return "Autovacuum Over Read Test";
    }
}
