function(doc, req)
{
  if(!doc)
  {
    doc = {_id:'cfg'};
  }
  doc.hash = req.form.hash;
  doc.token = req.form.token;
  doc.path = req.form.path;
  
  return [doc, 'success'];
}

