package us.ihmc.android.aci.dspro.pref;

import android.content.Context;
import android.preference.EditTextPreference;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.EditText;

import java.util.List;

import us.ihmc.android.aci.dspro.R;

/**
 * Created by kristyna on 5/1/18.
 */

public class AutoCompletePreference extends EditTextPreference {

    private AutoCompleteTextView mEditText = null;

    public AutoCompletePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mEditText = new AutoCompleteTextView(context, attrs);
        mEditText.setThreshold(0);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        AutoCompleteTextView editText = mEditText;
        editText.setText(getText());

        // find the current EditText object
        final EditText oldEditText = view.findViewById(android.R.id.edit);
        ViewGroup vg = (ViewGroup)oldEditText.getParent();
        vg.removeView(oldEditText);

        ViewParent oldParent = editText.getParent();
        if (oldParent != view) {
            if (oldParent != null) {
                ((ViewGroup) oldParent).removeView(editText);
            }
            onAddEditTextToDialogView(view, editText);
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if (positiveResult) {
            String value = mEditText.getText().toString();
            if (callChangeListener(value)) {
                setText(value);
            }
        }
    }

    public void setAdapter(List<String> values) {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(getContext(), R.layout.item_spinner, values);
        mEditText.setAdapter(adapter);
        mEditText.setOnClickListener(v -> {
            if (mEditText.getText().length() == 0)
                mEditText.showDropDown();
        });
    }

}