function(doc, req) 
{
  doc = 
  {
    _id:req.uuid, 
    time:(new Date()).getTime()/1000.0,
    channel:Number(req.form.chan), 
    id:Number(req.form.id),
    cumulative:Number(req.form.cumulative)
  }

  return [doc,'updated'];
}

