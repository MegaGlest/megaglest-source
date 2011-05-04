' Set your settings
strFileURL = "http://master.dl.sourceforge.net/project/megaglest/7z.exe"
strHDLocation = "..\..\data\glest_game\7z.exe"

WScript.Echo "----------------------------------------"
WScript.Echo "About to download 7z.exe from:"
WScript.Echo strFileURL & ", please wait..."

' Fetch the file
Set objXMLHTTP = CreateObject("MSXML2.XMLHTTP")
objXMLHTTP.open "GET", strFileURL, false
objXMLHTTP.send()

If objXMLHTTP.Status = 200 Then
Set objADOStream = CreateObject("ADODB.Stream")
objADOStream.Open
objADOStream.Type = 1 'adTypeBinary

objADOStream.Write objXMLHTTP.ResponseBody
objADOStream.Position = 0    'Set the stream position to the start

Set objFSO = Createobject("Scripting.FileSystemObject")
If objFSO.Fileexists(strHDLocation) Then objFSO.DeleteFile strHDLocation
Set objFSO = Nothing

objADOStream.SaveToFile strHDLocation
objADOStream.Close
Set objADOStream = Nothing
WScript.Echo "7z.exe has been downloaded successfully to: " 
WScript.Echo strHDLocation
WScript.Echo "----------------------------------------"
End if

Set objXMLHTTP = Nothing