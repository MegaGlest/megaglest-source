' Set your settings
strFileURL = "http://www.soft-haus.com/glest/tools/7z.exe"
strHDLocation = ".\7z.exe"

WScript.Echo "----------------------------------------"
WScript.Echo "About to download 7z.exe from:"
WScript.Echo strFileURL & ", please wait..."

' Fetch the file
Set objXMLHTTP =CreateObject("WinHttp.WinHttpRequest.5.1")
If objXMLHTTP Is Nothing Then Set objXMLHTTP = CreateObject("WinHttp.WinHttpRequest")
If objXMLHTTP Is Nothing Then Set objXMLHTTP = CreateObject("MSXML2.ServerXMLHTTP")
If objXMLHTTP Is Nothing Then Set objXMLHTTP = CreateObject("Microsoft.XMLHTTP")

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

' Set your settings
strFileURL = "http://www.soft-haus.com/glest/tools/7z.dll"
strHDLocation = ".\7z.dll"

WScript.Echo "----------------------------------------"
WScript.Echo "About to download 7z.dll from:"
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
  WScript.Echo "7z.dll has been downloaded successfully to: " 
  WScript.Echo strHDLocation
  WScript.Echo "----------------------------------------"
End if

Set objXMLHTTP = Nothing

' Set your settings
strFileURL = "http://www.soft-haus.com/glest/tools/wget.exe"
strHDLocation = ".\wget.exe"

WScript.Echo "----------------------------------------"
WScript.Echo "About to download wget.exe from:"
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
  WScript.Echo "wget.exe has been downloaded successfully to: " 
  WScript.Echo strHDLocation
  WScript.Echo "----------------------------------------"
End if

Set objXMLHTTP = Nothing

' Set your settings
strFileURL = "http://www.soft-haus.com/glest/tools/tar.exe"
strHDLocation = ".\tar.exe"

WScript.Echo "----------------------------------------"
WScript.Echo "About to download tar.exe from:"
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
  WScript.Echo "tar.exe has been downloaded successfully to: " 
  WScript.Echo strHDLocation
  WScript.Echo "----------------------------------------"
End if

Set objXMLHTTP = Nothing
