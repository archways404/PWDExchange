$Outlook = New-Object -ComObject Outlook.Application
$Namespace = $Outlook.GetNamespace("MAPI")
$Inbox = $Namespace.GetDefaultFolder([Microsoft.Office.Interop.Outlook.OlDefaultFolders]::olFolderInbox)
$Items = $Inbox.Items.Restrict("[Subject] = 'PWDExchange'")
$UnreadMessages = @()

foreach ($item in $Items) {
    if ($item.UnRead -eq $true) {
        $UnreadMessages += $item.Body
        # Mark as read
        $item.UnRead = $false
        $item.Save()
    }
}

# Output messages joined by a separator (e.g., \n---\n)
if ($UnreadMessages.Count -gt 0) {
    $UnreadMessages -join "`n---MSG---`n"
}