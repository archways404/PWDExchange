$To = $args[0]
$Subject = $args[1]
$Body = $args[2]

$Outlook = New-Object -ComObject Outlook.Application
$Mail = $Outlook.CreateItem(0)
$Mail.To = $To
$Mail.Subject = $Subject
$Mail.Body = $Body
$Mail.Send()