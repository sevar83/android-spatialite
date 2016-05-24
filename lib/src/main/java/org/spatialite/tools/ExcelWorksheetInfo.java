package org.spatialite.tools;

import android.os.Parcel;
import android.os.Parcelable;

public class ExcelWorksheetInfo implements Parcelable {
	public String name;
	public int rows;
	public int columns;

	public ExcelWorksheetInfo() {
	}

	protected ExcelWorksheetInfo(Parcel in) {
		name = in.readString();
		rows = in.readInt();
		columns = in.readInt();
	}

	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(name);
		dest.writeInt(rows);
		dest.writeInt(columns);
	}

	@SuppressWarnings("unused")
	public static final Parcelable.Creator<ExcelWorksheetInfo> CREATOR = new Parcelable.Creator<ExcelWorksheetInfo>() {
		@Override
		public ExcelWorksheetInfo createFromParcel(Parcel in) {
			return new ExcelWorksheetInfo(in);
		}

		@Override
		public ExcelWorksheetInfo[] newArray(int size) {
			return new ExcelWorksheetInfo[size];
		}
	};
}