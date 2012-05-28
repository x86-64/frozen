define(`SHOP',`
{ class = "end" },
{ class = "null", name = "$1" },
$2,
{ class = "shop/return" }
')
define(`SHOP_QUERY', `
{ class = "shop/pass", shop = (machine_t)"$1" }	
')
define(`SHOP_RETURN', `{ class = "shop/return" }')
