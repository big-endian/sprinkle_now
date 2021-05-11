//const char form_html[] PROGMEM = R"=====(
const char form_html_head[] = R"=====(
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Sprinkle Now!</title>
<link rel="stylesheet" type="text/css" href="./form.css" media="all">

</head>
<body id="main_body" >
  
  <div id="form_container">
  
    <h1><a>Sprinkler Zone Control</a></h1>
    <form id="zone_form" class="sprinklers"  method="post" action="/activate">
          <div class="form_description">
      <h2>Sprinkler Zone Control</h2>
      <p></p>
    </div>            
    <ul>
      
    <li id="li_1">
    <label class="description" for="zone">Activate Zones</label>
    <span>
    <!-- END FORM HEAD -->
)=====";


const char form_html_tail[] = R"=====(
    <!-- START FORM TAIL -->
    </span> 
    </li>
      
    <li class="buttons">
        <input id="saveForm" class="button_text" type="submit" name="submit" value="Submit" />
    </li>
      </ul>
    </form> 
  </div>
  </body>
</html>
)=====";
