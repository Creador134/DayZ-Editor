class EditorDZEFile: EditorFileType
{
	
	override EditorSaveData Import(string file, ImportSettings settings)
	{
		EditorSaveData save_data = new EditorSaveData();
		
		FileSerializer file_serializer = new FileSerializer();

		if (!FileExist(file)) {
			EditorLog.Error("File not found %1", file);
			return save_data;
		}
		
		if (!file_serializer.Open(file, FileMode.READ)) {
			EditorLog.Error("File in use %1", file);
			return save_data;
		}
		
		if (!file_serializer.Read(save_data)) {
			file_serializer.Close();
			EditorLog.Error("Unknown File Error %1", file);
			return save_data;
		}
		
		file_serializer.Close();
		
		// bugfix
		EditorSaveData bug_fix_save_data = new EditorSaveData();
		foreach (EditorObjectData object_data: save_data.EditorObjects) {
			bug_fix_save_data.EditorObjects.InsertData(EditorObjectData.Create(object_data.Type, object_data.Position, object_data.Orientation, object_data.Flags));
		}
		
		bug_fix_save_data.MapName = save_data.MapName;
		bug_fix_save_data.CameraPosition = save_data.CameraPosition;
		
		return bug_fix_save_data;
	}
	
	override void Export(EditorSaveData data, string file, ExportSettings settings)
	{
		if (FileExist(file) && !DeleteFile(file)) {
			return;
		}
		
		FileSerializer file_serializer = new FileSerializer();
		if (!file_serializer.Open(file, FileMode.WRITE)) {
			return;
		}
		
		if (!file_serializer.Write(data)) {
			file_serializer.Close();
			return;
		}
		
		file_serializer.Close();
	}
	
	override string GetExtension() {
		return ".dze";
	}
}