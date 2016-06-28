package org.spatialite.testsuite;

import android.content.Context;
import android.graphics.Color;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import org.spatialite.testsuite.tests.TestResult;

import java.util.List;

public class TestResultAdapter extends ArrayAdapter<TestResult> {

    public TestResultAdapter(Context context, List<TestResult> results) {
        super(context, 0, results);
    }

    @Override
    public View getView(int position, View view, ViewGroup parent) {

        ViewHolder holder;
        if(view == null){
            view = View.inflate(SpatialiteApplication.getInstance(), R.layout.test_result_row, null);
            holder = new ViewHolder();
            holder.testName = (TextView) view.findViewById(R.id.test_name);
            holder.testMessage = (TextView) view.findViewById(R.id.test_message);
            holder.testStatus = (TextView) view.findViewById(R.id.test_status);
            view.setTag(holder);
        } else {
            holder = (ViewHolder)view.getTag();
        }
        TestResult result = getItem(position);
        holder.testName.setText(result.getName());
        holder.testStatus.setText(result.toString());
        boolean messageAvailable = !isNullOrEmpty(result.getMessage()) || !isNullOrEmpty(result.getError());
        holder.testMessage.setVisibility(messageAvailable ? View.VISIBLE : View.GONE);
        if(messageAvailable){
            String message = isNullOrEmpty(result.getError()) ? result.getMessage() : result.getError();
            holder.testMessage.setText(message);
        }
        int displayColor = result.isSuccess() ? Color.GREEN : Color.RED;
        holder.testStatus.setTextColor(displayColor);
        return view;
    }

    private boolean isNullOrEmpty(String value){
        return value == null || value.length() == 0;
    }

    static class ViewHolder {
        TextView testName;
        TextView testMessage;
        TextView testStatus;
    }
}
